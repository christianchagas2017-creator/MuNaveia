// Testa o backend GLES do emulador de funcao fixa num contexto OpenGL ES 3 REAL
// (EGL sem janela; no sandbox e o Mesa llvmpipe). Cada cena e desenhada num FBO,
// lida de volta e comparada com o valor esperado calculado a mao.
//
// Build (Linux):
//   g++ -std=c++17 -Isource/Platform/GLES -I<khronos gl/api> -I<khronos egl/api>
//       tests/ff_gles_test.cpp -lEGL -lGLESv2 -o ff_gles_test
// Execucao: LIBGL_ALWAYS_SOFTWARE=1 ./ff_gles_test
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "GLESHeaders.h"
#include "FixedFunctionGLES.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>

using namespace Platform::FF;

static int g_fail = 0, g_checks = 0;
#define CHECK(c) do { ++g_checks; if (!(c)) { ++g_fail; std::printf("FALHOU %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

static const int W = 64, H = 64;

struct Px { int r, g, b, a; };
static Px At(int x, int y)
{
	unsigned char p[4] = { 0, 0, 0, 0 };
	glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p);
	return Px{ p[0], p[1], p[2], p[3] };
}
static bool Near(Px p, int r, int g, int b, int a, int tol = 2)
{
	return std::abs(p.r - r) <= tol && std::abs(p.g - g) <= tol && std::abs(p.b - b) <= tol && (a < 0 || std::abs(p.a - a) <= tol);
}
static bool Check(const char* what, Px p, int r, int g, int b, int a = -1, int tol = 2)
{
	bool ok = Near(p, r, g, b, a, tol);
	if (!ok) std::printf("  [%s] obtido %d %d %d %d, esperado %d %d %d %d\n", what, p.r, p.g, p.b, p.a, r, g, b, a);
	return ok;
}

static void Clear(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static GLuint MakeTex2x2(const unsigned char* rgba)
{
	GLuint t; glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	return t;
}

// Quad cobrindo a tela inteira em coordenadas ortograficas 0..1
static void FullQuad(Emulator& e, bool uv)
{
	e.Begin(kQuads);
	if (uv) e.TexCoord(0, 0);
	e.Vertex(0, 0);
	if (uv) e.TexCoord(1, 0);
	e.Vertex(1, 0);
	if (uv) e.TexCoord(1, 1);
	e.Vertex(1, 1);
	if (uv) e.TexCoord(0, 1);
	e.Vertex(0, 1);
	e.End();
}
static void Ortho01(Emulator& e)
{
	e.MatrixMode(kProjection); e.LoadIdentity(); e.Ortho2D(0, 1, 0, 1);
	e.MatrixMode(kModelView); e.LoadIdentity();
}

int main()
{
	// ---- contexto EGL sem janela ----
	PFNEGLGETPLATFORMDISPLAYEXTPROC gpd = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
	EGLDisplay dpy = gpd ? gpd(0x31DD, EGL_DEFAULT_DISPLAY, nullptr) : EGL_NO_DISPLAY;
	if (dpy == EGL_NO_DISPLAY) dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	EGLint maj, min;
	if (!eglInitialize(dpy, &maj, &min)) { std::printf("eglInitialize falhou\n"); return 2; }
	eglBindAPI(EGL_OPENGL_ES_API);
	const EGLint ca[] = { EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_NONE };
	EGLConfig cfg; EGLint n = 0;
	if (!eglChooseConfig(dpy, ca, &cfg, 1, &n) || n < 1) { std::printf("sem config ES3\n"); return 2; }
	const EGLint cx[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
	EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, cx);
	const EGLint pb[] = { EGL_WIDTH, W, EGL_HEIGHT, H, EGL_NONE };
	EGLSurface surf = eglCreatePbufferSurface(dpy, cfg, pb);
	if (!eglMakeCurrent(dpy, surf, surf, ctx)) { std::printf("eglMakeCurrent falhou\n"); return 2; }
	std::printf("GL: %s | %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));

	GLuint fbo, rb, db;
	glGenFramebuffers(1, &fbo); glGenRenderbuffers(1, &rb); glGenRenderbuffers(1, &db);
	glBindRenderbuffer(GL_RENDERBUFFER, rb); glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, W, H);
	glBindRenderbuffer(GL_RENDERBUFFER, db); glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, W, H);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, db);
	CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glViewport(0, 0, W, H);

	GLESBackend be;
	if (!be.Init()) { std::printf("Init falhou: %s\n", be.LastError().c_str()); return 1; }
	CHECK(be.Ready());
	Emulator e; e.SetBackend(&be);

	// 1) cor lisa
	{
		Clear(0, 0, 0, 1); Ortho01(e); e.Reset(); Ortho01(e);
		e.Color(1, 0, 0, 1); FullQuad(e, false);
		CHECK(Check("cor lisa", At(32, 32), 255, 0, 0, 255));
		CHECK(Check("cor lisa canto", At(0, 0), 255, 0, 0, 255));
		CHECK(Check("cor lisa canto2", At(W - 1, H - 1), 255, 0, 0, 255));
	}
	// 2) Gouraud: vermelho->azul na horizontal
	{
		Clear(0, 0, 0, 1); e.Reset(); Ortho01(e);
		e.Begin(kQuads);
		e.Color(1, 0, 0); e.Vertex(0, 0); e.Color(0, 0, 1); e.Vertex(1, 0);
		e.Color(0, 0, 1); e.Vertex(1, 1); e.Color(1, 0, 0); e.Vertex(0, 1);
		e.End();
		CHECK(Check("gouraud esquerda", At(1, 32), 250, 0, 4, 255, 6));
		CHECK(Check("gouraud meio", At(32, 32), 128, 0, 128, 255, 6));
		CHECK(Check("gouraud direita", At(62, 32), 4, 0, 250, 255, 6));
	}
	// 3) textura MODULATE: cor (1,.5,1) * textura 2x2
	{
		const unsigned char px[16] = { 255,255,255,255,  0,255,0,255,   255,0,0,255,  0,0,255,255 }; // (0,0) (1,0) / (0,1) (1,1)
		GLuint t = MakeTex2x2(px);
		Clear(0, 0, 0, 1); e.Reset(); Ortho01(e);
		e.Enable(kTexture2D); e.TexEnv(kTexEnv, kTexEnvMode, kModulate);
		e.Color(1, 0.5f, 1, 1); FullQuad(e, true);
		// y cresce para cima: v=0 embaixo
		CHECK(Check("mod tex(0,0) branco", At(16, 16), 255, 128, 255, 255));
		CHECK(Check("mod tex(1,0) verde", At(48, 16), 0, 128, 0, 255));
		CHECK(Check("mod tex(0,1) vermelho", At(16, 48), 255, 0, 0, 255));
		CHECK(Check("mod tex(1,1) azul", At(48, 48), 0, 0, 255, 255));
		glDeleteTextures(1, &t);
	}
	// 4) textura ADD: cor (0.6,0.6,0.6) + textura (0.5,1,0), saturando
	{
		const unsigned char px[16] = { 128,255,0,255, 128,255,0,255, 128,255,0,255, 128,255,0,255 };
		GLuint t = MakeTex2x2(px);
		Clear(0, 0, 0, 1); e.Reset(); Ortho01(e);
		e.Enable(kTexture2D); e.TexEnv(kTexEnv, kTexEnvMode, kAdd);
		e.Color(0.6f, 0.6f, 0.6f, 1); FullQuad(e, true);
		CHECK(Check("add", At(32, 32), 255, 255, 153, 255));
		glDeleteTextures(1, &t);
	}
	// 5) teste alfa GREATER 0.25: metade esquerda alfa 0.1 (some), direita 0.9 (fica)
	{
		Clear(0, 0, 1, 1); e.Reset(); Ortho01(e);
		e.Enable(kAlphaTest); e.AlphaFunc(kGreater, 0.25f);
		e.Begin(kQuads);
		e.Color(1, 1, 0, 0.1f); e.Vertex(0, 0); e.Color(1, 1, 0, 0.9f); e.Vertex(1, 0);
		e.Color(1, 1, 0, 0.9f); e.Vertex(1, 1); e.Color(1, 1, 0, 0.1f); e.Vertex(0, 1);
		e.End();
		CHECK(Check("alfa descarta", At(4, 32), 0, 0, 255, 255));
		CHECK(Check("alfa mantem", At(58, 32), 255, 255, 0, -1));
	}
	// 6) mistura (blend do driver) junto com o shader
	{
		Clear(0, 0, 1, 1); e.Reset(); Ortho01(e);
		glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		e.Color(1, 0, 0, 0.5f); FullQuad(e, false);
		glDisable(GL_BLEND);
		CHECK(Check("blend", At(32, 32), 128, 0, 128, -1, 3));
	}
	// 7) neblina: plano em perspectiva, z olho = -10, fog linear 0..20 => f=0.5
	{
		const float fogc[4] = { 0, 1, 0, 1 };
		Clear(0, 0, 0, 1); e.Reset();
		e.MatrixMode(kProjection); e.LoadIdentity(); e.Perspective(60, 1.0, 1.0, 100.0);
		e.MatrixMode(kModelView); e.LoadIdentity();
		e.Enable(kFog); e.Fogi(kFogMode, kLinear); e.Fogf(kFogStart, 0); e.Fogf(kFogEnd, 20); e.Fogfv(kFogColor, fogc);
		e.Color(1, 0, 0, 1);
		e.Begin(kQuads);
		e.Vertex(-20, -20, -10); e.Vertex(20, -20, -10); e.Vertex(20, 20, -10); e.Vertex(-20, 20, -10);
		e.End();
		CHECK(Check("fog linear", At(32, 32), 128, 128, 0, 255, 3));
		// EXP: f = exp(-0.1*10) = 0.3679
		Clear(0, 0, 0, 1); e.Fogi(kFogMode, kExp); e.Fogf(kFogDensity, 0.1f);
		e.Begin(kQuads);
		e.Vertex(-20, -20, -10); e.Vertex(20, -20, -10); e.Vertex(20, 20, -10); e.Vertex(-20, 20, -10);
		e.End();
		CHECK(Check("fog exp", At(32, 32), 94, 161, 0, 255, 3));
		// EXP2: f = exp(-(0.1*10)^2) = 0.3679 (mesmo valor com d=1)
		Clear(0, 0, 0, 1); e.Fogi(kFogMode, kExp2);
		e.Begin(kQuads);
		e.Vertex(-20, -20, -10); e.Vertex(20, -20, -10); e.Vertex(20, 20, -10); e.Vertex(-20, 20, -10);
		e.End();
		CHECK(Check("fog exp2", At(32, 32), 94, 161, 0, 255, 3));
		// EXP2 com densidade 0.05: d=0.5 => f=exp(-0.25)=0.7788
		Clear(0, 0, 0, 1); e.Fogf(kFogDensity, 0.05f);
		e.Begin(kQuads);
		e.Vertex(-20, -20, -10); e.Vertex(20, -20, -10); e.Vertex(20, 20, -10); e.Vertex(-20, 20, -10);
		e.End();
		CHECK(Check("fog exp2 b", At(32, 32), 199, 56, 0, 255, 3));
		e.Disable(kFog);
	}
	// 8) arrays de cliente (glVertexPointer & cia) com glDrawArrays
	{
		static const float pos[] = { 0,0, 1,0, 1,1, 0,1 };
		static const unsigned char col[] = { 0,255,0,255, 0,255,0,255, 0,255,0,255, 0,255,0,255 };
		Clear(0, 0, 0, 1); e.Reset(); Ortho01(e);
		e.EnableClientState(kVertexArray); e.EnableClientState(kColorArray);
		e.VertexPointer(2, kFloat, 0, pos); e.ColorPointer(4, kUnsignedByte, 0, col);
		e.DrawArrays(kQuads, 0, 4);
		CHECK(Check("arrays", At(32, 32), 0, 255, 0, 255));
		e.DisableClientState(kVertexArray); e.DisableClientState(kColorArray);
	}
	// 9) pilha de matrizes: translada metade da tela
	{
		Clear(0, 0, 0, 1); e.Reset(); Ortho01(e);
		e.Color(1, 1, 1, 1);
		e.PushMatrix(); e.Translate(0.5f, 0, 0); e.Scale(0.5f, 1, 1); FullQuad(e, false); e.PopMatrix();
		CHECK(Check("matriz vazio", At(16, 32), 0, 0, 0, 255));
		CHECK(Check("matriz cheio", At(48, 32), 255, 255, 255, 255));
	}
	// 10) profundidade (driver) e linhas
	{
		Clear(0, 0, 0, 1); e.Reset();
		e.MatrixMode(kProjection); e.LoadIdentity(); e.Ortho(0, 1, 0, 1, -1, 1);
		e.MatrixMode(kModelView); e.LoadIdentity();
		glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL);
		e.Color(1, 0, 0, 1); e.Begin(kQuads); e.Vertex(0, 0, -0.5f); e.Vertex(1, 0, -0.5f); e.Vertex(1, 1, -0.5f); e.Vertex(0, 1, -0.5f); e.End();
		e.Color(0, 0, 1, 1); e.Begin(kQuads); e.Vertex(0, 0, 0.5f); e.Vertex(1, 0, 0.5f); e.Vertex(1, 1, 0.5f); e.Vertex(0, 1, 0.5f); e.End();
		glDisable(GL_DEPTH_TEST);
		// ortografica olhando para -z: z=+0.5 (azul) esta mais perto que z=-0.5 (vermelho)
		CHECK(Check("profundidade", At(32, 32), 0, 0, 255, 255));
	}
	// 11) POLYGON -> leque, TRIANGLE_STRIP e QUAD_STRIP
	{
		Clear(0, 0, 0, 1); e.Reset(); Ortho01(e);
		e.Color(1, 1, 0, 1);
		e.Begin(kPolygon); e.Vertex(0, 0); e.Vertex(1, 0); e.Vertex(1, 1); e.Vertex(0, 1); e.End();
		CHECK(Check("polygon", At(10, 50), 255, 255, 0, 255));
		Clear(0, 0, 0, 1); e.Color(0, 1, 1, 1);
		e.Begin(kQuadStrip); e.Vertex(0, 0); e.Vertex(0, 1); e.Vertex(1, 0); e.Vertex(1, 1); e.End();
		CHECK(Check("quadstrip", At(50, 10), 0, 255, 255, 255));
	}
	CHECK(glGetError() == GL_NO_ERROR);
	CHECK(e.errors == 0);

	be.Shutdown();
	std::printf("%d verificacoes, %d falhas\n", g_checks, g_fail);
	std::printf(g_fail ? "FALHA\n" : "OK\n");
	return g_fail ? 1 : 0;
}
