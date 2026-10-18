// Testes do nucleo de emulacao da funcao fixa (sem GL):
//   g++ -std=c++17 -Wall -Wextra -Isource -o ff_core_test tests/ff_core_test.cpp && ./ff_core_test
#include "Platform/GLES/FixedFunction.h"
#include <cstdio>
#include <cmath>
#include <vector>

using namespace Platform::FF;

static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FALHOU: %s (linha %d)\n", #c, __LINE__); ++fails; } } while (0)
static bool Near(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) <= eps; }

struct Mock : Backend
{
	std::vector<DrawBatch> calls;
	void Draw(const DrawBatch& b) override { calls.push_back(b); }
};

// Transforma (x,y,z,1) por uma matriz coluna-principal.
static void Xf(const Mat4& m, float x, float y, float z, float out[4])
{
	for (int r = 0; r < 4; ++r) out[r] = m.m[0 * 4 + r] * x + m.m[1 * 4 + r] * y + m.m[2 * 4 + r] * z + m.m[3 * 4 + r];
}

int main()
{
	Mock mock; Emulator gl(&mock);
	float o[4];

	// ---------- matrizes ----------
	gl.Translate(10, 20, 30);
	Xf(gl.ModelView(), 1, 2, 3, o); CHECK(Near(o[0], 11) && Near(o[1], 22) && Near(o[2], 33) && Near(o[3], 1));

	// ordem: glTranslate(10,0,0) depois glScale(2,2,2) => escala primeiro, depois translada: (1,0,0) -> (12,0,0)
	gl.LoadIdentity(); gl.Translate(10, 0, 0); gl.Scale(2, 2, 2);
	Xf(gl.ModelView(), 1, 0, 0, o); CHECK(Near(o[0], 12));
	// ordem inversa: scale depois translate => (1,0,0) -> (22,0,0)
	gl.LoadIdentity(); gl.Scale(2, 2, 2); gl.Translate(10, 0, 0);
	Xf(gl.ModelView(), 1, 0, 0, o); CHECK(Near(o[0], 22));

	// rotacao de 90 graus em Z leva X em Y; em Y leva Z em X
	gl.LoadIdentity(); gl.Rotate(90, 0, 0, 1);
	Xf(gl.ModelView(), 1, 0, 0, o); CHECK(Near(o[0], 0) && Near(o[1], 1));
	gl.LoadIdentity(); gl.Rotate(90, 0, 1, 0);
	Xf(gl.ModelView(), 0, 0, 1, o); CHECK(Near(o[0], 1) && Near(o[2], 0));
	gl.LoadIdentity(); gl.Rotate(90, 1, 0, 0);
	Xf(gl.ModelView(), 0, 1, 0, o); CHECK(Near(o[1], 0) && Near(o[2], 1));
	gl.LoadIdentity(); gl.Rotate(45, 0, 0, 5);   // eixo nao unitario e normalizado
	Xf(gl.ModelView(), 1, 0, 0, o); CHECK(Near(o[0], std::sqrt(0.5f)) && Near(o[1], std::sqrt(0.5f)));
	// 90 graus: TODOS os vetores da base em cada eixo (regra da mao direita)
	{
		struct Case { float ax, ay, az; float in[3]; float expect[3]; };
		const Case cs[] = {
			{ 0,0,1, {1,0,0}, { 0, 1,0} }, { 0,0,1, {0,1,0}, {-1, 0,0} }, { 0,0,1, {0,0,1}, {0,0,1} },
			{ 1,0,0, {0,1,0}, { 0, 0,1} }, { 1,0,0, {0,0,1}, { 0,-1,0} }, { 1,0,0, {1,0,0}, {1,0,0} },
			{ 0,1,0, {0,0,1}, { 1, 0,0} }, { 0,1,0, {1,0,0}, { 0, 0,-1} }, { 0,1,0, {0,1,0}, {0,1,0} },
		};
		for (const Case& c : cs)
		{
			gl.LoadIdentity(); gl.Rotate(90, c.ax, c.ay, c.az);
			Xf(gl.ModelView(), c.in[0], c.in[1], c.in[2], o);
			CHECK(Near(o[0], c.expect[0]) && Near(o[1], c.expect[1]) && Near(o[2], c.expect[2]));
		}
		// eixo diagonal (1,1,1), 120 graus: permutacao ciclica x->y->z->x
		gl.LoadIdentity(); gl.Rotate(120, 1, 1, 1);
		Xf(gl.ModelView(), 1, 0, 0, o); CHECK(Near(o[0], 0) && Near(o[1], 1) && Near(o[2], 0));
		Xf(gl.ModelView(), 0, 1, 0, o); CHECK(Near(o[0], 0) && Near(o[1], 0) && Near(o[2], 1));
		Xf(gl.ModelView(), 0, 0, 1, o); CHECK(Near(o[0], 1) && Near(o[1], 0) && Near(o[2], 0));
		// a rotacao preserva comprimento e e invertivel: R(a) * R(-a) = identidade
		gl.LoadIdentity(); gl.Rotate(37, 0.3f, -0.5f, 0.8f); gl.Rotate(-37, 0.3f, -0.5f, 0.8f);
		Xf(gl.ModelView(), 1, 2, 3, o); CHECK(Near(o[0], 1, 1e-3f) && Near(o[1], 2, 1e-3f) && Near(o[2], 3, 1e-3f));
		gl.LoadIdentity(); gl.Rotate(37, 0.3f, -0.5f, 0.8f);
		Xf(gl.ModelView(), 1, 2, 3, o); CHECK(Near(o[0]*o[0] + o[1]*o[1] + o[2]*o[2], 14.0f, 1e-3f));
	}
	gl.LoadIdentity(); gl.Rotate(30, 0, 0, 0);   // eixo nulo: nada acontece
	Xf(gl.ModelView(), 1, 2, 3, o); CHECK(Near(o[0], 1) && Near(o[1], 2) && Near(o[2], 3));

	// pilha
	gl.LoadIdentity(); gl.Translate(5, 0, 0);
	gl.PushMatrix(); gl.Translate(0, 7, 0);
	Xf(gl.ModelView(), 0, 0, 0, o); CHECK(Near(o[0], 5) && Near(o[1], 7));
	gl.PopMatrix();
	Xf(gl.ModelView(), 0, 0, 0, o); CHECK(Near(o[0], 5) && Near(o[1], 0));
	int e0 = gl.errors; gl.PopMatrix(); gl.PopMatrix(); CHECK(gl.errors > e0); // pop alem da base e erro, nao crasha

	// modos de matriz sao pilhas separadas
	gl.LoadIdentity(); gl.MatrixMode(kProjection); gl.LoadIdentity(); gl.Ortho2D(0, 640, 480, 0);
	Xf(gl.Projection(), 0, 0, 0, o);      CHECK(Near(o[0], -1) && Near(o[1], 1));    // canto superior esquerdo
	Xf(gl.Projection(), 640, 480, 0, o);  CHECK(Near(o[0], 1) && Near(o[1], -1));    // canto inferior direito
	Xf(gl.ModelView(), 1, 2, 3, o);       CHECK(Near(o[0], 1) && Near(o[1], 2));     // modelview intacta
	gl.MatrixMode(kModelView);

	// perspectiva: ponto no plano near vai para z_ndc=-1, no far para +1; o centro fica no centro
	gl.MatrixMode(kProjection); gl.LoadIdentity(); gl.Perspective(90, 2.0, 1.0, 100.0);
	{
		float c[4]; Xf(gl.Projection(), 0, 0, -1, c);    CHECK(Near(c[2] / c[3], -1.0f));
		Xf(gl.Projection(), 0, 0, -100, c);              CHECK(Near(c[2] / c[3], 1.0f, 1e-3f));
		Xf(gl.Projection(), 1, 0, -1, c);                CHECK(Near(c[0] / c[3], 0.5f));  // fov 90 => tan=1; aspecto 2 => x/2
		Xf(gl.Projection(), 0, 1, -1, c);                CHECK(Near(c[1] / c[3], 1.0f));
	}
	gl.LoadIdentity(); gl.MatrixMode(kModelView); gl.LoadIdentity();

	// glGetFloatv
	gl.Translate(1, 2, 3);
	{ float m[16]; CHECK(gl.GetFloatv(kModelViewMatrix, m) && Near(m[12], 1) && Near(m[13], 2) && Near(m[14], 3)); }
	gl.Color(0.1f, 0.2f, 0.3f, 0.4f);
	{ float c[4]; CHECK(gl.GetFloatv(kCurrentColor, c) && Near(c[0], 0.1f) && Near(c[3], 0.4f)); CHECK(!gl.GetFloatv(0x1234, c)); }
	gl.LoadIdentity();

	// ---------- modo imediato ----------
	mock.calls.clear(); gl.Color(1, 1, 1, 1);
	gl.Begin(kQuads);
	gl.Color(1, 0, 0, 1); gl.TexCoord(0, 0); gl.Vertex(0, 0);
	gl.Color(0, 1, 0, 1); gl.TexCoord(1, 0); gl.Vertex(1, 0);
	gl.Color(0, 0, 1, 1); gl.TexCoord(1, 1); gl.Vertex(1, 1);
	gl.Color(1, 1, 0, 1); gl.TexCoord(0, 1); gl.Vertex(0, 1, 5);
	gl.End();
	CHECK(mock.calls.size() == 1);
	if (mock.calls.size() == 1)
	{
		const DrawBatch& b = mock.calls[0];
		CHECK(b.mode == kTriangles && b.vertices.size() == 6);   // quad -> 2 triangulos
		// (0,1,2) e (0,2,3)
		CHECK(Near(b.vertices[0].x, 0) && Near(b.vertices[1].x, 1) && Near(b.vertices[2].y, 1));
		CHECK(Near(b.vertices[3].x, 0) && Near(b.vertices[4].x, 1) && Near(b.vertices[4].y, 1) && Near(b.vertices[5].y, 1) && Near(b.vertices[5].z, 5));
		// cada vertice leva a cor/textura vigente na hora do glVertex
		CHECK(Near(b.vertices[0].r, 1) && Near(b.vertices[0].g, 0));
		CHECK(Near(b.vertices[1].g, 1) && Near(b.vertices[1].u, 1) && Near(b.vertices[1].v, 0));
		CHECK(Near(b.vertices[2].b, 1) && Near(b.vertices[2].u, 1) && Near(b.vertices[2].v, 1));
		CHECK(Near(b.vertices[5].r, 1) && Near(b.vertices[5].g, 1) && Near(b.vertices[5].b, 0));
	}

	// quad incompleto e descartado; 2 quads viram 12 vertices
	mock.calls.clear();
	gl.Begin(kQuads); for (int i = 0; i < 3; ++i) gl.Vertex((float)i, 0); gl.End();
	CHECK(mock.calls.empty());
	gl.Begin(kQuads); for (int i = 0; i < 9; ++i) gl.Vertex((float)i, 0); gl.End();
	CHECK(mock.calls.size() == 1 && mock.calls[0].vertices.size() == 12);

	// demais primitivas
	mock.calls.clear();
	gl.Begin(kTriangleFan); for (int i = 0; i < 5; ++i) gl.Vertex((float)i, 0); gl.End();
	CHECK(mock.calls.size() == 1 && mock.calls[0].mode == kTriangleFan && mock.calls[0].vertices.size() == 5);
	gl.Begin(kLines); for (int i = 0; i < 5; ++i) gl.Vertex((float)i, 0); gl.End();   // 5 vertices: usa 4
	CHECK(mock.calls.size() == 2 && mock.calls[1].mode == kLines && mock.calls[1].vertices.size() == 4);
	gl.Begin(kLineStrip); for (int i = 0; i < 3; ++i) gl.Vertex((float)i, 0); gl.End();
	CHECK(mock.calls.size() == 3 && mock.calls[2].mode == kLineStrip);
	gl.Begin(kTriangles); for (int i = 0; i < 7; ++i) gl.Vertex((float)i, 0); gl.End(); // usa 6
	CHECK(mock.calls.size() == 4 && mock.calls[3].vertices.size() == 6);
	gl.Begin(kPolygon); for (int i = 0; i < 4; ++i) gl.Vertex((float)i, 0); gl.End();
	CHECK(mock.calls.size() == 5 && mock.calls[4].mode == kTriangleFan);
	gl.Begin(kTriangleFan); gl.Vertex(0, 0); gl.Vertex(1, 0); gl.End();                // incompleto
	CHECK(mock.calls.size() == 5);

	// uso invalido nao desenha nem derruba
	e0 = gl.errors;
	gl.Vertex(1, 1); gl.End(); gl.Begin(kQuads); gl.Begin(kTriangles); gl.End();
	CHECK(gl.errors >= e0 + 3);
	mock.calls.clear();

	// ---------- arrays ----------
	const float verts3[] = { 0,0,0,  1,0,0,  1,1,0,  0,1,0 };
	const float uvs[]    = { 0,0, 1,0, 1,1, 0,1 };
	const float cols4[]  = { 1,0,0,1,  0,1,0,1,  0,0,1,1,  1,1,1,0.5f };
	gl.EnableClientState(kVertexArray); gl.VertexPointer(3, kFloat, 0, verts3);
	gl.EnableClientState(kTexCoordArray); gl.TexCoordPointer(2, kFloat, 0, uvs);
	gl.EnableClientState(kColorArray); gl.ColorPointer(4, kFloat, 0, cols4);
	gl.DrawArrays(kQuads, 0, 4);
	CHECK(mock.calls.size() == 1 && mock.calls[0].vertices.size() == 6);
	if (mock.calls.size() == 1)
	{
		const DrawBatch& b = mock.calls[0];
		CHECK(Near(b.vertices[2].x, 1) && Near(b.vertices[2].y, 1));
		CHECK(Near(b.vertices[2].u, 1) && Near(b.vertices[2].v, 1));
		CHECK(Near(b.vertices[5].a, 0.5f) && Near(b.vertices[5].u, 0) && Near(b.vertices[5].v, 1));
	}

	// tamanho 2 => z=0; cor tamanho 3 => alfa=1; 'first' desloca
	mock.calls.clear();
	const float verts2[] = { 10,10,  20,10,  20,20,  30,30,  40,40,  50,50 };
	const float cols3[]  = { 1,0,0,  0,1,0,  0,0,1,  0.5f,0.5f,0.5f,  0.1f,0.2f,0.3f,  1,1,1 };
	gl.DisableClientState(kTexCoordArray);   // o array de textura anterior so tem 4 vertices
	gl.VertexPointer(2, kFloat, 0, verts2); gl.ColorPointer(3, kFloat, 0, cols3);
	gl.DrawArrays(kTriangles, 3, 3);
	CHECK(mock.calls.size() == 1 && mock.calls[0].vertices.size() == 3);
	if (mock.calls.size() == 1)
	{
		const DrawBatch& b = mock.calls[0];
		CHECK(Near(b.vertices[0].x, 30) && Near(b.vertices[0].z, 0) && Near(b.vertices[2].x, 50));
		CHECK(Near(b.vertices[0].r, 0.5f) && Near(b.vertices[1].b, 0.3f) && Near(b.vertices[1].a, 1.0f));
	}

	// sem array de cor: usa a cor atual; sem array de textura: usa a textura atual
	mock.calls.clear();
	gl.DisableClientState(kColorArray); gl.DisableClientState(kTexCoordArray);
	gl.Color(0.9f, 0.8f, 0.7f, 0.6f); gl.TexCoord(0.25f, 0.75f);
	gl.DrawArrays(kTriangles, 0, 3);
	CHECK(mock.calls.size() == 1);
	if (mock.calls.size() == 1)
	{
		const Vertex& v = mock.calls[0].vertices[1];
		CHECK(Near(v.r, 0.9f) && Near(v.a, 0.6f) && Near(v.u, 0.25f) && Near(v.v, 0.75f));
	}

	// stride (vertices intercalados: x,y,z,u,v)
	mock.calls.clear();
	const float inter[] = { 1,2,3, 0.1f,0.2f,  4,5,6, 0.3f,0.4f,  7,8,9, 0.5f,0.6f };
	gl.EnableClientState(kTexCoordArray);
	gl.VertexPointer(3, kFloat, 5 * sizeof(float), inter);
	gl.TexCoordPointer(2, kFloat, 5 * sizeof(float), inter + 3);
	gl.DrawArrays(kTriangles, 0, 3);
	CHECK(mock.calls.size() == 1);
	if (mock.calls.size() == 1)
	{
		const DrawBatch& b = mock.calls[0];
		CHECK(Near(b.vertices[1].x, 4) && Near(b.vertices[1].z, 6) && Near(b.vertices[2].u, 0.5f) && Near(b.vertices[2].v, 0.6f));
	}

	// cor em bytes e normalizada
	mock.calls.clear();
	const unsigned char cb[] = { 255,0,0,255,  0,255,0,128,  0,0,255,0 };
	gl.EnableClientState(kColorArray); gl.ColorPointer(4, kUnsignedByte, 0, cb);
	gl.VertexPointer(2, kFloat, 0, verts2);
	gl.DrawArrays(kTriangles, 0, 3);
	CHECK(mock.calls.size() == 1 && Near(mock.calls[0].vertices[0].r, 1) && Near(mock.calls[0].vertices[1].g, 1) && Near(mock.calls[0].vertices[1].a, 128 / 255.0f));

	// array de vertices desligado: nada e desenhado
	mock.calls.clear();
	gl.DisableClientState(kVertexArray); gl.DrawArrays(kTriangles, 0, 3);
	CHECK(mock.calls.empty());
	gl.EnableClientState(kVertexArray);

	// ---------- estado entregue ao shader ----------
	mock.calls.clear();
	gl.DisableClientState(kColorArray); gl.DisableClientState(kTexCoordArray);
	gl.Enable(kTexture2D); gl.Enable(kAlphaTest); gl.AlphaFunc(kGreater, 0.25f);
	gl.Enable(kFog); gl.Fogf(kFogMode, (float)kLinear); gl.Fogf(kFogStart, 2000); gl.Fogf(kFogEnd, 4000);
	{ const float fc[4] = { 0.1f, 0.2f, 0.3f, 1.0f }; gl.Fogfv(kFogColor, fc); }
	gl.TexEnv(kTexEnv, kTexEnvMode, kAdd);
	gl.DrawArrays(kTriangles, 0, 3);
	CHECK(mock.calls.size() == 1);
	if (mock.calls.size() == 1)
	{
		const DrawState& s = mock.calls[0].state;
		CHECK(s.texture2D && s.texEnv == kAdd && s.alphaFunc == kGreater && Near(s.alphaRef, 0.25f));
		CHECK(s.fogMode == kLinear && Near(s.fogStart, 2000) && Near(s.fogEnd, 4000) && Near(s.fogColor[2], 0.3f));
	}
	// desligar teste alfa e neblina zera no estado entregue
	mock.calls.clear();
	gl.Disable(kAlphaTest); gl.Disable(kFog); gl.Disable(kTexture2D);
	gl.DrawArrays(kTriangles, 0, 3);
	CHECK(mock.calls.size() == 1 && mock.calls[0].state.alphaFunc == 0 && mock.calls[0].state.fogMode == 0 && !mock.calls[0].state.texture2D);
	CHECK(!gl.Enable(0x0BE2 /* GL_BLEND: nao emulado, vai ao driver */));

	// padroes do OpenGL: neblina comeca em EXP, densidade 1; texenv MODULATE; alfa SEMPRE
	{
		Mock m2; Emulator g2(&m2);
		g2.EnableClientState(kVertexArray); g2.VertexPointer(2, kFloat, 0, verts2);
		g2.Enable(kFog); g2.Enable(kAlphaTest);
		g2.DrawArrays(kTriangles, 0, 3);
		CHECK(m2.calls.size() == 1 && m2.calls[0].state.fogMode == kExp && Near(m2.calls[0].state.fogDensity, 1.0f)
			&& m2.calls[0].state.texEnv == kModulate && m2.calls[0].state.alphaFunc == kAlways);
	}

	// mvp = projecao * modelview (nessa ordem)
	mock.calls.clear();
	gl.MatrixMode(kProjection); gl.LoadIdentity(); gl.Scale(2, 2, 2);
	gl.MatrixMode(kModelView);  gl.LoadIdentity(); gl.Translate(1, 0, 0);
	gl.DrawArrays(kTriangles, 0, 3);
	CHECK(mock.calls.size() == 1);
	if (mock.calls.size() == 1) { Xf(mock.calls[0].state.mvp, 0, 0, 0, o); CHECK(Near(o[0], 2)); }   // (0+1)*2

	// valores invalidos nao quebram o estado
	e0 = gl.errors; gl.AlphaFunc(0x9999, 0.5f); gl.TexEnv(0x1, 0x2, kModulate); gl.Fogf(0x7777, 1); CHECK(gl.errors == e0 + 3);

	if (fails == 0) std::printf("OK: nucleo FixedFunction passou em todos os testes.\n");
	return fails ? 1 : 0;
}
