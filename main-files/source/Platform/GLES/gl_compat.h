#pragma once

// Camada de compatibilidade: o codigo do jogo continua chamando glBegin/glColor4f/
// glMatrixMode/glEnable(GL_FOG)... e aqui essas chamadas sao redirecionadas para o
// emulador de funcao fixa (FixedFunction.h) + backend OpenGL ES 3.
//
// So deve ser incluido fora do Windows (no Windows o jogo usa o OpenGL classico).
// Incluir DEPOIS dos headers de GL ES (GLESHeaders.h) e antes de qualquer codigo do jogo.
//
// As funcoes reais do ES continuam acessiveis dentro de namespace Platform::GL
// (chamadas com "::gl..." antes dos macros abaixo serem definidos).

#if defined(_WIN32)
#error "gl_compat.h e so para plataformas sem OpenGL de funcao fixa (Android/iOS/Linux ES)"
#endif

#include "GLESHeaders.h"
#include "FixedFunctionGLES.h"

// ---------------------------------------------------------------------------
// Constantes que existem no OpenGL classico e nao no ES 3
// ---------------------------------------------------------------------------
#include "GLLegacyEnums.h"

namespace Platform
{
	namespace GL
	{
		// ---- estado global do emulador (uma thread de render, como no jogo) ----
		struct Context
		{
			Platform::FF::GLESBackend backend;
			Platform::FF::Emulator    emu;
			void (*swap)() = nullptr;     // troca de buffers (SDL_GL_SwapWindow)
			bool ready = false;
		};
		inline Context& Ctx() { static Context c; return c; }
		inline Platform::FF::Emulator& Emu() { return Ctx().emu; }

		// Chamar uma vez com o contexto GLES ja ativo. false = ver LastError().
		inline bool Init(void (*swap)() = nullptr)
		{
			Context& c = Ctx();
			c.swap = swap;
			if (!c.backend.Init()) return false;
			c.emu.SetBackend(&c.backend);
			c.emu.Reset();
			c.ready = true;
			return true;
		}
		inline const std::string& LastError() { return Ctx().backend.LastError(); }
		inline void Shutdown() { Context& c = Ctx(); c.backend.Shutdown(); c.ready = false; }
		inline void SwapBuffers() { if (Ctx().swap) Ctx().swap(); }

		// ---- modo imediato ----
		inline void Begin(unsigned m)                         { Emu().Begin(m); }
		inline void End()                                     { Emu().End(); }
		inline void Vertex2f(float x, float y)                { Emu().Vertex(x, y, 0.f); }
		inline void Vertex3f(float x, float y, float z)      { Emu().Vertex(x, y, z); }
		inline void Vertex3fv(const float* v)                 { Emu().Vertex(v[0], v[1], v[2]); }
		inline void Color3f(float r, float g, float b)       { Emu().Color(r, g, b, 1.f); }
		inline void Color4f(float r, float g, float b, float a) { Emu().Color(r, g, b, a); }
		inline void Color3fv(const float* c)                  { Emu().Color(c[0], c[1], c[2], 1.f); }
		inline void Color3ub(unsigned char r, unsigned char g, unsigned char b) { Emu().Color4ub(r, g, b, 255); }
		inline void Color4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a) { Emu().Color4ub(r, g, b, a); }
		inline void TexCoord2f(float u, float v)              { Emu().TexCoord(u, v); }
		inline void Normal3f(float, float, float)             {} // sem iluminacao: normais nao sao usadas

		// ---- matrizes ----
		inline void MatrixMode(unsigned m)  { Emu().MatrixMode(m); }
		inline void LoadIdentity()          { Emu().LoadIdentity(); }
		inline void PushMatrix()            { Emu().PushMatrix(); }
		inline void PopMatrix()             { Emu().PopMatrix(); }
		inline void Translatef(float x, float y, float z)            { Emu().Translate(x, y, z); }
		inline void Scalef(float x, float y, float z)                { Emu().Scale(x, y, z); }
		inline void Rotatef(float a, float x, float y, float z)      { Emu().Rotate(a, x, y, z); }
		inline void gluPerspective_(double fovy, double aspect, double n, double f) { Emu().Perspective(fovy, aspect, n, f); }
		inline void gluOrtho2D_(double l, double r, double b, double t)             { Emu().Ortho2D(l, r, b, t); }

		// ---- arrays de vertices ----
		inline void EnableClientState(unsigned c)  { Emu().EnableClientState(c); }
		inline void DisableClientState(unsigned c) { Emu().DisableClientState(c); }
		inline void VertexPointer(int n, unsigned t, int s, const void* p)   { Emu().VertexPointer(n, t, s, p); }
		inline void ColorPointer(int n, unsigned t, int s, const void* p)    { Emu().ColorPointer(n, t, s, p); }
		inline void TexCoordPointer(int n, unsigned t, int s, const void* p) { Emu().TexCoordPointer(n, t, s, p); }
		inline void DrawArrays(unsigned mode, int first, int count)           { Emu().DrawArrays(mode, first, count); }

		// ---- estados ----
		// Estados emulados (textura, teste alfa, neblina) nao vao ao driver; os de
		// extensoes ARB do OpenGL classico sao ignorados; o resto passa direto.
		inline bool IsIgnoredCap(unsigned cap)
		{
			return cap == GL_MULTISAMPLE_ARB || cap == GL_VERTEX_PROGRAM_ARB || cap == GL_FRAGMENT_PROGRAM_ARB;
		}
		inline void Enable(unsigned cap)
		{
			if (Emu().Enable(cap, true) || IsIgnoredCap(cap)) return;
			::glEnable(cap);
		}
		inline void Disable(unsigned cap)
		{
			if (Emu().Enable(cap, false) || IsIgnoredCap(cap)) return;
			::glDisable(cap);
		}
		inline void AlphaFunc(unsigned f, float ref) { Emu().AlphaFunc(f, ref); }
		inline void Fogf(unsigned p, float v)        { Emu().Fogf(p, v); }
		inline void Fogi(unsigned p, int v)          { Emu().Fogi(p, v); }
		inline void Fogfv(unsigned p, const float* v) { Emu().Fogfv(p, v); }
		inline void TexEnvf(unsigned t, unsigned p, float v) { Emu().TexEnv(t, p, (unsigned)v); }
		inline void TexEnvi(unsigned t, unsigned p, int v)   { Emu().TexEnv(t, p, (unsigned)v); }
		inline void PolygonMode(unsigned, unsigned) {}  // so usado em depuracao (wireframe)
		inline void PushAttrib(unsigned) {}
		inline void PopAttrib() {}

		inline void GetFloatv(unsigned pname, float* out)
		{
			if (Emu().GetFloatv(pname, out)) return;
			::glGetFloatv(pname, out);
		}

		// ---- texturas ----
		inline void TexParameteri(unsigned target, unsigned pname, int v)
		{
			if (v == GL_CLAMP) v = GL_CLAMP_TO_EDGE; // GL_CLAMP nao existe no ES
			::glTexParameteri(target, pname, v);
		}
		// O jogo passa 3/4 (contagem de componentes) como formato interno; o ES exige
		// o mesmo formato do dado. Linhas de pixels nao sao alinhadas a 4 bytes.
		inline void TexImage2D(unsigned target, int level, int internalformat, int w, int h, int border,
			unsigned format, unsigned type, const void* data)
		{
			(void)internalformat;
			::glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			::glTexImage2D(target, level, (GLint)format, w, h, border, format, type, data);
		}

		// glReadPixels: o ES so garante GL_RGBA/GL_UNSIGNED_BYTE; RGB e convertido.
		// Profundidade nao pode ser lida no ES 3: devolve 1.0 (= "visivel"), que e o
		// resultado seguro para o teste de ocultacao de TestDepthBuffer.
		inline void ReadPixels(int x, int y, int w, int h, unsigned format, unsigned type, void* out)
		{
			if (format == GL_DEPTH_COMPONENT)
			{
				if (type == GL_FLOAT) { float* f = (float*)out; for (int i = 0; i < w * h; ++i) f[i] = 1.0f; }
				return;
			}
			if (format == GL_RGB && type == GL_UNSIGNED_BYTE)
			{
				std::vector<unsigned char> tmp((size_t)w * (size_t)h * 4);
				::glPixelStorei(GL_PACK_ALIGNMENT, 1);
				::glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, tmp.data());
				unsigned char* o = (unsigned char*)out;
				for (size_t i = 0; i < (size_t)w * (size_t)h; ++i) { o[3 * i] = tmp[4 * i]; o[3 * i + 1] = tmp[4 * i + 1]; o[3 * i + 2] = tmp[4 * i + 2]; }
				return;
			}
			::glReadPixels(x, y, w, h, format, type, out);
		}

		// ---- GLU ----
		inline void* gluNewQuadric_() { return nullptr; }
		inline void  gluSphere_(void*, double, int, int) {} // so depuracao de colisao (PhysicsManager)
	}
}

// ---------------------------------------------------------------------------
// Macros: os nomes classicos passam a chamar a camada acima.
// (Definidos por ultimo para que o codigo acima ainda use as funcoes reais.)
// ---------------------------------------------------------------------------
#define glBegin(m)                      ::Platform::GL::Begin(m)
#define glEnd()                         ::Platform::GL::End()
#define glVertex2f(x, y)                ::Platform::GL::Vertex2f((float)(x), (float)(y))
#define glVertex3f(x, y, z)             ::Platform::GL::Vertex3f((float)(x), (float)(y), (float)(z))
#define glVertex3fv(v)                  ::Platform::GL::Vertex3fv(v)
#define glColor3f(r, g, b)              ::Platform::GL::Color3f((float)(r), (float)(g), (float)(b))
#define glColor4f(r, g, b, a)           ::Platform::GL::Color4f((float)(r), (float)(g), (float)(b), (float)(a))
#define glColor3fv(c)                   ::Platform::GL::Color3fv(c)
#define glColor3ub(r, g, b)             ::Platform::GL::Color3ub((unsigned char)(r), (unsigned char)(g), (unsigned char)(b))
#define glColor4ub(r, g, b, a)          ::Platform::GL::Color4ub((unsigned char)(r), (unsigned char)(g), (unsigned char)(b), (unsigned char)(a))
#define glTexCoord2f(u, v)              ::Platform::GL::TexCoord2f((float)(u), (float)(v))
#define glNormal3f(x, y, z)             ::Platform::GL::Normal3f((float)(x), (float)(y), (float)(z))

#define glMatrixMode(m)                 ::Platform::GL::MatrixMode(m)
#define glLoadIdentity()                ::Platform::GL::LoadIdentity()
#define glPushMatrix()                  ::Platform::GL::PushMatrix()
#define glPopMatrix()                   ::Platform::GL::PopMatrix()
#define glTranslatef(x, y, z)           ::Platform::GL::Translatef((float)(x), (float)(y), (float)(z))
#define glScalef(x, y, z)               ::Platform::GL::Scalef((float)(x), (float)(y), (float)(z))
#define glRotatef(a, x, y, z)           ::Platform::GL::Rotatef((float)(a), (float)(x), (float)(y), (float)(z))
#define gluPerspective(f, a, n, fr)     ::Platform::GL::gluPerspective_((double)(f), (double)(a), (double)(n), (double)(fr))
#define gluOrtho2D(l, r, b, t)          ::Platform::GL::gluOrtho2D_((double)(l), (double)(r), (double)(b), (double)(t))

#define glEnableClientState(c)          ::Platform::GL::EnableClientState(c)
#define glDisableClientState(c)         ::Platform::GL::DisableClientState(c)
#define glVertexPointer(n, t, s, p)     ::Platform::GL::VertexPointer(n, t, s, p)
#define glColorPointer(n, t, s, p)      ::Platform::GL::ColorPointer(n, t, s, p)
#define glTexCoordPointer(n, t, s, p)   ::Platform::GL::TexCoordPointer(n, t, s, p)
#define glDrawArrays(m, f, c)           ::Platform::GL::DrawArrays(m, f, c)

#define glEnable(c)                     ::Platform::GL::Enable(c)
#define glDisable(c)                    ::Platform::GL::Disable(c)
#define glAlphaFunc(f, r)               ::Platform::GL::AlphaFunc(f, (float)(r))
#define glFogf(p, v)                    ::Platform::GL::Fogf(p, (float)(v))
#define glFogi(p, v)                    ::Platform::GL::Fogi(p, (int)(v))
#define glFogfv(p, v)                   ::Platform::GL::Fogfv(p, v)
#define glTexEnvf(t, p, v)              ::Platform::GL::TexEnvf(t, p, (float)(v))
#define glTexEnvi(t, p, v)              ::Platform::GL::TexEnvi(t, p, (int)(v))
#define glPolygonMode(f, m)             ::Platform::GL::PolygonMode(f, m)
#define glPushAttrib(m)                 ::Platform::GL::PushAttrib(m)
#define glPopAttrib()                   ::Platform::GL::PopAttrib()
#define glGetFloatv(p, o)               ::Platform::GL::GetFloatv(p, o)

#define glTexParameteri(t, p, v)        ::Platform::GL::TexParameteri(t, p, v)
#define glTexImage2D(t, l, i, w, h, b, f, ty, d) ::Platform::GL::TexImage2D(t, l, i, w, h, b, f, ty, d)
#define glReadPixels(x, y, w, h, f, t, o) ::Platform::GL::ReadPixels(x, y, w, h, f, t, o)

#define gluNewQuadric()                 ::Platform::GL::gluNewQuadric_()
#define gluSphere(q, r, s, k)           ::Platform::GL::gluSphere_(q, r, s, k)
