// Teste diferencial: as mesmas cenas (gl_diff_scenes.inc) sao desenhadas
//   -DREF_GL  : pelo OpenGL classico (Mesa, perfil de compatibilidade) = referencia
//   (padrao)  : por gl_compat.h + emulador + backend OpenGL ES 3
// e gravam os pixels em <dir>/<cena>.rgba para comparacao (gl_diff_compare.py).
//
// Uso: gl_diff_ref|gl_diff_emu <diretorio_de_saida>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>

#ifdef REF_GL
// ---- OpenGL classico: sem gl.h no sandbox, declara-se o que o jogo usa ----
#include <GLES3/gl3.h>            // tipos e as funcoes modernas (mesmos simbolos do libGL)
#include "GLLegacyEnums.h"
extern "C" {
	void glBegin(GLenum); void glEnd(); void glVertex2f(GLfloat, GLfloat); void glVertex3f(GLfloat, GLfloat, GLfloat);
	void glVertex3fv(const GLfloat*); void glColor3f(GLfloat, GLfloat, GLfloat); void glColor4f(GLfloat, GLfloat, GLfloat, GLfloat);
	void glColor3fv(const GLfloat*); void glColor3ub(GLubyte, GLubyte, GLubyte); void glColor4ub(GLubyte, GLubyte, GLubyte, GLubyte);
	void glTexCoord2f(GLfloat, GLfloat); void glNormal3f(GLfloat, GLfloat, GLfloat);
	void glMatrixMode(GLenum); void glLoadIdentity(); void glPushMatrix(); void glPopMatrix();
	void glTranslatef(GLfloat, GLfloat, GLfloat); void glScalef(GLfloat, GLfloat, GLfloat); void glRotatef(GLfloat, GLfloat, GLfloat, GLfloat);
	void glFrustum(double, double, double, double, double, double); void glOrtho(double, double, double, double, double, double);
	void glEnableClientState(GLenum); void glDisableClientState(GLenum);
	void glVertexPointer(GLint, GLenum, GLsizei, const void*); void glColorPointer(GLint, GLenum, GLsizei, const void*);
	void glTexCoordPointer(GLint, GLenum, GLsizei, const void*);
	void glAlphaFunc(GLenum, GLfloat); void glFogf(GLenum, GLfloat); void glFogi(GLenum, GLint); void glFogfv(GLenum, const GLfloat*);
	void glTexEnvf(GLenum, GLenum, GLfloat); void glTexEnvi(GLenum, GLenum, GLint);
	void glPolygonMode(GLenum, GLenum);
}
// gluPerspective / gluOrtho2D conforme a especificacao do GLU (a referencia nao usa o codigo do emulador)
static void RefPerspective(double fovy, double aspect, double zn, double zf)
{
	double top = zn * std::tan(fovy * 3.14159265358979323846 / 360.0), right = top * aspect;
	glFrustum(-right, right, -top, top, zn, zf);
}
#define gluPerspective(f, a, n, fr) RefPerspective(f, a, n, fr)
#define gluOrtho2D(l, r, b, t) glOrtho(l, r, b, t, -1.0, 1.0)
#else
#include "gl_compat.h"
#endif

#include "gl_diff_scenes.inc"

int main(int argc, char** argv)
{
	if (argc < 2) { std::printf("uso: %s <dir>\n", argv[0]); return 2; }
	const std::string dir = argv[1];
	mkdir(dir.c_str(), 0755);

	PFNEGLGETPLATFORMDISPLAYEXTPROC gpd = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
	EGLDisplay dpy = gpd ? gpd(0x31DD, EGL_DEFAULT_DISPLAY, nullptr) : EGL_NO_DISPLAY;
	if (dpy == EGL_NO_DISPLAY) dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	EGLint maj, min;
	if (!eglInitialize(dpy, &maj, &min)) { std::printf("eglInitialize falhou\n"); return 2; }
#ifdef REF_GL
	eglBindAPI(EGL_OPENGL_API);
	const EGLint ca[] = { EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_NONE };
	const EGLint cx[] = { EGL_NONE };   // sem pedir versao: perfil de compatibilidade
#else
	eglBindAPI(EGL_OPENGL_ES_API);
	const EGLint ca[] = { EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_NONE };
	const EGLint cx[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
#endif
	EGLConfig cfg; EGLint n = 0;
	if (!eglChooseConfig(dpy, ca, &cfg, 1, &n) || n < 1) { std::printf("sem config\n"); return 2; }
	EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, cx);
	const EGLint pb[] = { EGL_WIDTH, SW, EGL_HEIGHT, SH, EGL_NONE };
	EGLSurface surf = eglCreatePbufferSurface(dpy, cfg, pb);
	if (ctx == EGL_NO_CONTEXT || !eglMakeCurrent(dpy, surf, surf, ctx)) { std::printf("contexto falhou 0x%x\n", eglGetError()); return 2; }
	std::printf("%s\n", (const char*)glGetString(GL_VERSION));

	GLuint fbo, rb, db;
	glGenFramebuffers(1, &fbo); glGenRenderbuffers(1, &rb); glGenRenderbuffers(1, &db);
	glBindRenderbuffer(GL_RENDERBUFFER, rb); glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, SW, SH);
	glBindRenderbuffer(GL_RENDERBUFFER, db); glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, SW, SH);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, db);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { std::printf("FBO incompleto\n"); return 2; }

#ifndef REF_GL
	if (!Platform::GL::Init()) { std::printf("Init falhou: %s\n", Platform::GL::LastError().c_str()); return 1; }
#endif

	int errs = 0;
	for (const Scene& s : kScenes)
	{
		while (glGetError() != GL_NO_ERROR) {}
		s.fn();
		std::vector<unsigned char> px((size_t)SW * SH * 4);
		glReadPixels(0, 0, SW, SH, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
		GLenum e = glGetError();
		if (e != GL_NO_ERROR) { std::printf("  %s: glGetError=0x%x\n", s.name, e); ++errs; }
		FILE* f = fopen((dir + "/" + s.name + ".rgba").c_str(), "wb");
		fwrite(px.data(), 1, px.size(), f); fclose(f);
	}
#ifndef REF_GL
	if (Platform::GL::Emu().errors) { std::printf("emulador registrou %d erros\n", Platform::GL::Emu().errors); ++errs; }
#endif
	std::printf("%zu cenas, %d erros\n", sizeof(kScenes) / sizeof(kScenes[0]), errs);
	return errs ? 1 : 0;
}
