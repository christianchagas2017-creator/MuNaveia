// Cenas escritas EXATAMENTE como o jogo escreve (glBegin/glColor4f/glMatrixMode...).
// Compiladas duas vezes: contra o OpenGL classico (referencia, -DREF_GL) e contra
// gl_compat.h + OpenGL ES (emulacao). Os pixels de cada cena sao comparados.
#include <cmath>
#include <vector>

static const int SW = 96, SH = 96;

// Com -DREF_CLAMP_EDGE a referencia usa CLAMP_TO_EDGE no lugar de GL_CLAMP: prova que a unica
// diferenca em cenas com GL_CLAMP+LINEAR e a semantica da borda.
#ifdef REF_CLAMP_EDGE
#undef GL_CLAMP
#define GL_CLAMP 0x812F
#endif

static void Reset3D()
{
	glDisable(GL_TEXTURE_2D); glDisable(GL_BLEND); glDisable(GL_ALPHA_TEST); glDisable(GL_FOG);
	glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glDepthMask(1);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	glViewport(0, 0, SW, SH);
}
static void ClearScene(float r, float g, float b)
{
	Reset3D();
	glClearColor(r, g, b, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
static void Persp(float fov, float aspect, float n, float f) { glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspective(fov, aspect, n, f); glMatrixMode(GL_MODELVIEW); glLoadIdentity(); }
static void Ortho2D(float l, float r, float b, float t) { glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluOrtho2D(l, r, b, t); glMatrixMode(GL_MODELVIEW); glLoadIdentity(); }

// Textura de teste deterministica. comps = 3 (RGB) ou 4 (RGBA). Como o jogo: formato interno = numero de componentes.
static unsigned int MakeTexture(int w, int h, int comps, unsigned filter, unsigned wrap)
{
	std::vector<unsigned char> d((size_t)w * h * comps);
	for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x)
	{
		unsigned char* p = &d[((size_t)y * w + x) * comps];
		bool chk = ((x / 2) + (y / 2)) & 1;
		p[0] = chk ? 255 : (unsigned char)(x * 255 / (w - 1));
		p[1] = chk ? 200 : (unsigned char)(y * 255 / (h - 1));
		p[2] = chk ? 40 : 160;
		if (comps == 4) p[3] = (unsigned char)(x * 255 / (w - 1));
	}
	unsigned int t; glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
#ifdef REF_GL
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);   // a referencia nao tem o gancho que faz isso
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, comps, w, h, 0, comps == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, d.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	return t;
}

typedef void (*SceneFn)();
struct Scene { const char* name; SceneFn fn; };

// 0: quadrilatero com degrade (Gouraud) e glColor4ub
static void S_Gradient()
{
	ClearScene(0.1f, 0.1f, 0.1f); Ortho2D(0, 1, 0, 1);
	glBegin(GL_QUADS);
	glColor3f(1, 0, 0); glVertex2f(0.1f, 0.1f);
	glColor3f(0, 1, 0); glVertex2f(0.9f, 0.1f);
	glColor4ub(0, 0, 255, 255); glVertex2f(0.9f, 0.9f);
	glColor3ub(255, 255, 0); glVertex2f(0.1f, 0.9f);
	glEnd();
}
// 1: leque texturizado RGB repetido, MODULATE, filtro LINEAR
static void S_TexFanRepeat()
{
	ClearScene(0, 0, 0); Ortho2D(0, 1, 0, 1);
	unsigned t = MakeTexture(16, 16, 3, GL_LINEAR, GL_REPEAT);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1.f, 0.8f, 1.f, 1.f);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(1.5f, 1.5f); glVertex3f(0.5f, 0.5f, 0);
	glTexCoord2f(0, 0); glVertex3f(0.05f, 0.05f, 0);
	glTexCoord2f(3, 0); glVertex3f(0.95f, 0.05f, 0);
	glTexCoord2f(3, 3); glVertex3f(0.95f, 0.95f, 0);
	glTexCoord2f(0, 3); glVertex3f(0.05f, 0.95f, 0);
	glTexCoord2f(0, 0); glVertex3f(0.05f, 0.05f, 0);
	glEnd();
	glDeleteTextures(1, &t);
}
// 2: partícula aditiva: TexEnv ADD + blend ONE,ONE, textura RGBA
static void S_AddParticle()
{
	ClearScene(0.2f, 0.1f, 0.3f); Ortho2D(0, 1, 0, 1);
	unsigned t = MakeTexture(8, 8, 4, GL_NEAREST, GL_CLAMP);
	glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
	glColor4f(0.3f, 0.2f, 0.1f, 1.f);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(0.1f, 0.1f); glTexCoord2f(1, 0); glVertex2f(0.9f, 0.1f);
	glTexCoord2f(1, 1); glVertex2f(0.9f, 0.9f); glTexCoord2f(0, 1); glVertex2f(0.1f, 0.9f);
	glEnd();
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glDeleteTextures(1, &t);
}
// 3: teste alfa GREATER 0.25 sobre textura RGBA com alfa em rampa + blend normal
static void S_AlphaTest()
{
	ClearScene(0, 0, 0.5f); Ortho2D(0, 1, 0, 1);
	unsigned t = MakeTexture(16, 16, 4, GL_NEAREST, GL_CLAMP);
	glEnable(GL_TEXTURE_2D); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, 0.25f);
	glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1, 1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(0.05f, 0.05f); glTexCoord2f(1, 0); glVertex2f(0.95f, 0.05f);
	glTexCoord2f(1, 1); glVertex2f(0.95f, 0.95f); glTexCoord2f(0, 1); glVertex2f(0.05f, 0.95f);
	glEnd();
	glDeleteTextures(1, &t);
}
// 4..6: neblina LINEAR/EXP/EXP2 sobre chao em perspectiva
static void FogScene(int mode)
{
	ClearScene(0.3f, 0.3f, 0.4f);
	Persp(60, 1.f, 1.f, 400.f);
	glRotatef(25, 1, 0, 0); glTranslatef(0, -30, 0);
	const float fc[4] = { 0.3f, 0.3f, 0.4f, 1.f };
	glEnable(GL_FOG); glFogf(GL_FOG_MODE, (float)mode);
	glFogf(GL_FOG_START, 20.f); glFogf(GL_FOG_END, 220.f); glFogf(GL_FOG_DENSITY, 0.012f);
	glFogfv(GL_FOG_COLOR, fc);
	unsigned t = MakeTexture(16, 16, 3, GL_LINEAR, GL_REPEAT);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex3f(-300, 0, 20); glTexCoord2f(12, 0); glVertex3f(300, 0, 20);
	glTexCoord2f(12, 40); glVertex3f(300, 0, -400); glTexCoord2f(0, 40); glVertex3f(-300, 0, -400);
	glEnd();
	glDeleteTextures(1, &t);
}
static void S_FogLinear() { FogScene(GL_LINEAR); }
static void S_FogExp()    { FogScene(GL_EXP); }
static void S_FogExp2()   { FogScene(GL_EXP2); }

// 7: arrays de cliente intercalados com GL_TRIANGLES (como os modelos BMD)
static void S_Arrays()
{
	ClearScene(0, 0, 0); Ortho2D(0, 1, 0, 1);
	static const float pos[] = { 0.1f,0.1f,0, 0.9f,0.1f,0, 0.5f,0.9f,0,   0.1f,0.95f,0, 0.4f,0.95f,0, 0.25f,0.6f,0 };
	static const unsigned char col[] = { 255,0,0,255, 0,255,0,255, 0,0,255,255,   255,255,255,255, 255,128,0,255, 0,255,255,255 };
	static const float uv[] = { 0,0, 1,0, 0.5f,1,   0,0, 1,0, 0.5f,1 };
	unsigned t = MakeTexture(8, 8, 3, GL_NEAREST, GL_REPEAT);
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_COLOR_ARRAY); glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, pos); glColorPointer(4, GL_UNSIGNED_BYTE, 0, col); glTexCoordPointer(2, GL_FLOAT, 0, uv);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableClientState(GL_VERTEX_ARRAY); glDisableClientState(GL_COLOR_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDeleteTextures(1, &t);
}
// 8: hierarquia de matrizes + profundidade + culling (cubo girando, dois cubos)
static void Cube(float r, float g, float b)
{
	static const float v[8][3] = { {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1} };
	static const int f[6][4] = { {0,3,2,1},{4,5,6,7},{0,1,5,4},{3,7,6,2},{0,4,7,3},{1,2,6,5} }; // anti-horario visto de fora
	glBegin(GL_QUADS);
	for (int i = 0; i < 6; ++i) { glColor3f(r * (0.5f + 0.1f * i), g * (0.5f + 0.1f * i), b * (0.5f + 0.1f * i)); for (int k = 0; k < 4; ++k) glVertex3fv(v[f[i][k]]); }
	glEnd();
}
static void S_Matrices()
{
	ClearScene(0.05f, 0.05f, 0.1f);
	Persp(50, 1.f, 1.f, 100.f);
	glEnable(GL_DEPTH_TEST); glEnable(GL_CULL_FACE);
	glTranslatef(0, 0, -9);
	glRotatef(25, 1, 0, 0); glRotatef(-40, 0, 1, 0); glRotatef(15, 0, 0, 1);
	glPushMatrix();
	glTranslatef(-1.6f, 0, 0); glScalef(1.0f, 0.7f, 1.3f); glRotatef(30, 0, 1, 0);
	Cube(1, 0.4f, 0.2f);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(1.7f, 0.3f, 0.6f); glRotatef(70, 1, 1, 0);
	Cube(0.2f, 0.7f, 1.f);
	glPopMatrix();
}
// 9: linhas (GL_LINES, LINE_STRIP, LINE_LOOP)
static void S_Lines()
{
	ClearScene(0, 0, 0); Ortho2D(0, 96, 0, 96);
	glColor3f(1, 1, 0);
	glBegin(GL_LINES); glVertex2f(5, 5); glVertex2f(90, 20); glVertex2f(10, 90); glVertex2f(80, 60); glEnd();
	glColor3f(0, 1, 1);
	glBegin(GL_LINE_STRIP); glVertex2f(10, 30); glVertex2f(40, 70); glVertex2f(60, 30); glVertex2f(85, 80); glEnd();
	glColor3f(1, 0, 1);
	glBegin(GL_LINE_LOOP); glVertex2f(20, 20); glVertex2f(70, 25); glVertex2f(60, 70); glVertex2f(25, 65); glEnd();
}
// 10: interface 2D em pixels (origem no canto superior esquerdo), sprite com blend
static void S_UI()
{
	ClearScene(0.2f, 0.2f, 0.2f); Ortho2D(0, 96, 96, 0);
	unsigned t = MakeTexture(16, 16, 4, GL_LINEAR, GL_CLAMP);
	glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.f, 0.f, 0.f, 0.6f);
	glBegin(GL_QUADS); glVertex2f(5, 5); glVertex2f(60, 5); glVertex2f(60, 40); glVertex2f(5, 40); glEnd();
	glEnable(GL_TEXTURE_2D);
	glColor4f(1, 1, 1, 0.9f);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0, 0); glVertex2f(30, 30); glTexCoord2f(1, 0); glVertex2f(90, 30);
	glTexCoord2f(1, 1); glVertex2f(90, 90); glTexCoord2f(0, 1); glVertex2f(30, 90);
	glEnd();
	glDeleteTextures(1, &t);
}
// 11: terreno: textura + neblina + teste alfa + profundidade + blend (combinacao)
static void S_Combo()
{
	ClearScene(0.4f, 0.5f, 0.6f);
	Persp(55, 1.f, 1.f, 300.f);
	glEnable(GL_DEPTH_TEST);
	glRotatef(-20, 1, 0, 0); glTranslatef(0, -12, -5);
	const float fc[4] = { 0.4f, 0.5f, 0.6f, 1.f };
	glEnable(GL_FOG); glFogf(GL_FOG_MODE, (float)GL_LINEAR); glFogf(GL_FOG_START, 10.f); glFogf(GL_FOG_END, 150.f); glFogfv(GL_FOG_COLOR, fc);
	unsigned t = MakeTexture(16, 16, 4, GL_LINEAR, GL_REPEAT);
	glEnable(GL_TEXTURE_2D); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, 0.1f);
	glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for (int i = 0; i < 4; ++i)
	{
		glPushMatrix(); glTranslatef(-30.f + i * 20.f, 0, -20.f - i * 25.f);
		glColor4f(1, 1, 1, 1);
		glBegin(GL_TRIANGLE_FAN);
		glTexCoord2f(0.5f, 0.5f); glVertex3f(0, 8, 0);
		glTexCoord2f(0, 0); glVertex3f(-10, 0, 0); glTexCoord2f(1, 0); glVertex3f(10, 0, 0);
		glTexCoord2f(1, 1); glVertex3f(10, 16, 0); glTexCoord2f(0, 1); glVertex3f(-10, 16, 0); glTexCoord2f(0, 0); glVertex3f(-10, 0, 0);
		glEnd();
		glPopMatrix();
	}
	glDeleteTextures(1, &t);
}
// 12: GL_CLAMP com filtro LINEAR (inventario do jogo): diferenca conhecida nas bordas
static void S_ClampLinear()
{
	ClearScene(0, 0, 0); Ortho2D(0, 1, 0, 1);
	unsigned t = MakeTexture(8, 8, 3, GL_LINEAR, GL_CLAMP);
	glEnable(GL_TEXTURE_2D); glColor3f(1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(-0.5f, -0.5f); glVertex2f(0.05f, 0.05f); glTexCoord2f(1.5f, -0.5f); glVertex2f(0.95f, 0.05f);
	glTexCoord2f(1.5f, 1.5f); glVertex2f(0.95f, 0.95f); glTexCoord2f(-0.5f, 1.5f); glVertex2f(0.05f, 0.95f);
	glEnd();
	glDeleteTextures(1, &t);
}
// 13: matriz atual lida de volta e usada de novo (glGetFloatv MODELVIEW) + cor atual
static void S_GetFloat()
{
	ClearScene(0, 0, 0); Ortho2D(0, 1, 0, 1);
	glPushMatrix(); glTranslatef(0.25f, 0.25f, 0); glScalef(0.5f, 0.5f, 1);
	float m[16]; glGetFloatv(GL_MODELVIEW_MATRIX, m);
	glPopMatrix();
	glColor4f(0.2f, 0.9f, 0.4f, 1);
	float c[4] = { 0.f, 0.f, 1.f, 1.f };
	glGetFloatv(GL_CURRENT_COLOR, c);   // o que acabou de ser definido
	// desenha usando a matriz lida: deve cair em 0.25..0.75
	glBegin(GL_QUADS);
	glColor3f(c[0], c[1], c[2]);
	glVertex2f(m[12], m[13]); glVertex2f(m[12] + m[0], m[13]); glVertex2f(m[12] + m[0], m[13] + m[5]); glVertex2f(m[12], m[13] + m[5]);
	glEnd();
}

// 14: glColor3ub deve ter alfa 255 (com blend SRC_ALPHA o quadrado fica opaco) e plano distante recorta geometria
static void S_ColorUbFar()
{
	ClearScene(0, 0, 0.3f); Persp(60, 1.f, 5.f, 40.f);
	glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor3ub(250, 120, 20);
	glBegin(GL_QUADS);   // perto: aparece inteiro
	glVertex3f(-6, -6, -20); glVertex3f(0, -6, -20); glVertex3f(0, 6, -20); glVertex3f(-6, 6, -20);
	glEnd();
	glColor4ub(20, 220, 120, 255);
	glBegin(GL_TRIANGLES);   // atravessa o plano distante (z=-40): deve ser recortado
	glVertex3f(1, -8, -20); glVertex3f(8, -8, -60); glVertex3f(1, 8, -20);
	glEnd();
}

static const Scene kScenes[] = {
	{ "gradient", S_Gradient }, { "texfan_repeat", S_TexFanRepeat }, { "add_particle", S_AddParticle },
	{ "alpha_test", S_AlphaTest }, { "fog_linear", S_FogLinear }, { "fog_exp", S_FogExp }, { "fog_exp2", S_FogExp2 },
	{ "arrays", S_Arrays }, { "matrices", S_Matrices }, { "lines", S_Lines }, { "ui", S_UI }, { "combo", S_Combo },
	{ "clamp_linear", S_ClampLinear }, { "getfloat", S_GetFloat }, { "ub_far", S_ColorUbFar },
};
