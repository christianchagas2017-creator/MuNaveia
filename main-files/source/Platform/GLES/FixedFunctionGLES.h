#pragma once

// Backend OpenGL ES 3.0 do emulador de funcao fixa: um unico shader reproduz
// textura (MODULATE/ADD/REPLACE/DECAL), teste alfa e neblina (LINEAR/EXP/EXP2);
// cada lote de vertices vai para um VBO de streaming.
//
// Requer um contexto GLES 3 ativo. Inclua GLESHeaders.h (ou os headers da
// plataforma) antes deste arquivo.

#include "FixedFunction.h"
#include <string>
#include <cstdio>

namespace Platform
{
	namespace FF
	{
		static_assert(sizeof(Vertex) == 9 * sizeof(float), "Vertex precisa ser compacto: 9 floats");

		// GLSL ES 3.00. Textos expostos para os testes poderem validar o shader em outras implementacoes.
		static const char* const kVertexShader =
			"#version 300 es\n"
			"layout(location = 0) in vec3 aPos;\n"
			"layout(location = 1) in vec4 aColor;\n"
			"layout(location = 2) in vec2 aUV;\n"
			"uniform mat4 uMVP;\n"
			"uniform mat4 uMV;\n"
			"out vec4 vColor;\n"
			"out vec2 vUV;\n"
			"out float vFogZ;\n"
			"void main() {\n"
			"    vec4 p = vec4(aPos, 1.0);\n"
			"    gl_Position = uMVP * p;\n"
			"    vColor = clamp(aColor, 0.0, 1.0);\n"
			"    vUV = aUV;\n"
			"    vFogZ = abs((uMV * p).z);\n"   // distancia no espaco do olho, como a funcao fixa
			"}\n";

		static const char* const kFragmentShader =
			"#version 300 es\n"
			"precision highp float;\n"
			"in vec4 vColor;\n"
			"in vec2 vUV;\n"
			"in float vFogZ;\n"
			"uniform sampler2D uTex;\n"
			"uniform int uUseTex;\n"      // 0 = sem textura
			"uniform int uTexEnv;\n"      // 0 MODULATE, 1 ADD, 2 REPLACE, 3 DECAL
			"uniform int uAlphaFunc;\n"   // 0 = desligado; 1..8 = NEVER..ALWAYS
			"uniform float uAlphaRef;\n"
			"uniform int uFogMode;\n"     // 0 = desligada; 1 LINEAR, 2 EXP, 3 EXP2
			"uniform float uFogStart;\n"
			"uniform float uFogEnd;\n"
			"uniform float uFogDensity;\n"
			"uniform vec4 uFogColor;\n"
			"out vec4 oColor;\n"
			"void main() {\n"
			"    vec4 c = vColor;\n"
			"    if (uUseTex != 0) {\n"
			"        vec4 t = texture(uTex, vUV);\n"
			"        if (uTexEnv == 0)      c = c * t;\n"
			"        else if (uTexEnv == 1) c = vec4(min(c.rgb + t.rgb, vec3(1.0)), c.a * t.a);\n"
			"        else if (uTexEnv == 2) c = t;\n"
			"        else                   c = vec4(mix(c.rgb, t.rgb, t.a), c.a);\n"
			"    }\n"
			"    if (uFogMode != 0) {\n"
			"        float f;\n"
			"        if (uFogMode == 1)      f = (uFogEnd - vFogZ) / (uFogEnd - uFogStart);\n"
			"        else if (uFogMode == 2) f = exp(-uFogDensity * vFogZ);\n"
			"        else { float d = uFogDensity * vFogZ; f = exp(-d * d); }\n"
			"        f = clamp(f, 0.0, 1.0);\n"
			"        c.rgb = mix(uFogColor.rgb, c.rgb, f);\n"
			"    }\n"
			"    if (uAlphaFunc != 0) {\n"
			"        bool pass;\n"
			"        if      (uAlphaFunc == 1) pass = false;\n"
			"        else if (uAlphaFunc == 2) pass = c.a <  uAlphaRef;\n"
			"        else if (uAlphaFunc == 3) pass = c.a == uAlphaRef;\n"
			"        else if (uAlphaFunc == 4) pass = c.a <= uAlphaRef;\n"
			"        else if (uAlphaFunc == 5) pass = c.a >  uAlphaRef;\n"
			"        else if (uAlphaFunc == 6) pass = c.a != uAlphaRef;\n"
			"        else if (uAlphaFunc == 7) pass = c.a >= uAlphaRef;\n"
			"        else                      pass = true;\n"
			"        if (!pass) discard;\n"
			"    }\n"
			"    oColor = c;\n"
			"}\n";

		class GLESBackend : public Backend
		{
		public:
			GLESBackend() {}
			~GLESBackend() { Shutdown(); }

			// Compila o shader e cria os buffers. false = ver LastError().
			bool Init()
			{
				if (m_program) return true;
				GLuint vs = Compile(GL_VERTEX_SHADER, kVertexShader);
				GLuint fs = vs ? Compile(GL_FRAGMENT_SHADER, kFragmentShader) : 0;
				if (!vs || !fs) { if (vs) glDeleteShader(vs); if (fs) glDeleteShader(fs); return false; }

				m_program = glCreateProgram();
				glAttachShader(m_program, vs);
				glAttachShader(m_program, fs);
				glLinkProgram(m_program);
				glDeleteShader(vs); glDeleteShader(fs);

				GLint ok = 0;
				glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
				if (!ok)
				{
					char log[1024]; GLsizei n = 0; glGetProgramInfoLog(m_program, sizeof log, &n, log);
					m_error = std::string("link: ") + log;
					glDeleteProgram(m_program); m_program = 0;
					return false;
				}

				uMVP = glGetUniformLocation(m_program, "uMVP");
				uMV = glGetUniformLocation(m_program, "uMV");
				uTex = glGetUniformLocation(m_program, "uTex");
				uUseTex = glGetUniformLocation(m_program, "uUseTex");
				uTexEnv = glGetUniformLocation(m_program, "uTexEnv");
				uAlphaFunc = glGetUniformLocation(m_program, "uAlphaFunc");
				uAlphaRef = glGetUniformLocation(m_program, "uAlphaRef");
				uFogMode = glGetUniformLocation(m_program, "uFogMode");
				uFogStart = glGetUniformLocation(m_program, "uFogStart");
				uFogEnd = glGetUniformLocation(m_program, "uFogEnd");
				uFogDensity = glGetUniformLocation(m_program, "uFogDensity");
				uFogColor = glGetUniformLocation(m_program, "uFogColor");

				glGenVertexArrays(1, &m_vao);
				glGenBuffers(1, &m_vbo);
				glBindVertexArray(m_vao);
				glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
				const GLsizei stride = (GLsizei)sizeof(Vertex);
				glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void*)offsetof(Vertex, x));
				glEnableVertexAttribArray(1); glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (const void*)offsetof(Vertex, r));
				glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (const void*)offsetof(Vertex, u));
				return true;
			}

			void Shutdown()
			{
				if (m_vbo) { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
				if (m_vao) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
				if (m_program) { glDeleteProgram(m_program); m_program = 0; }
			}

			const std::string& LastError() const { return m_error; }
			bool Ready() const { return m_program != 0; }

			void Draw(const DrawBatch& b) override
			{
				if (!m_program || b.vertices.empty()) return;

				glUseProgram(m_program);
				glBindVertexArray(m_vao);
				glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
				// Reenvia a cada lote (o buffer antigo e "orfao": nao espera a GPU).
				glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(b.vertices.size() * sizeof(Vertex)), b.vertices.data(), GL_STREAM_DRAW);

				const DrawState& s = b.state;
				glUniformMatrix4fv(uMVP, 1, GL_FALSE, s.mvp.m);
				glUniformMatrix4fv(uMV, 1, GL_FALSE, s.modelView.m);
				glUniform1i(uTex, 0);
				glUniform1i(uUseTex, s.texture2D ? 1 : 0);
				glUniform1i(uTexEnv, s.texEnv == kAdd ? 1 : (s.texEnv == kReplace ? 2 : (s.texEnv == kDecal ? 3 : 0)));
				glUniform1i(uAlphaFunc, s.alphaFunc ? (GLint)(s.alphaFunc - kNever + 1) : 0);
				glUniform1f(uAlphaRef, s.alphaRef);
				glUniform1i(uFogMode, s.fogMode == kLinear ? 1 : (s.fogMode == kExp ? 2 : (s.fogMode == kExp2 ? 3 : 0)));
				glUniform1f(uFogStart, s.fogStart);
				glUniform1f(uFogEnd, s.fogEnd);
				glUniform1f(uFogDensity, s.fogDensity);
				glUniform4fv(uFogColor, 1, s.fogColor);

				glDrawArrays((GLenum)b.mode, 0, (GLsizei)b.vertices.size());
			}

		private:
			GLuint m_program = 0, m_vao = 0, m_vbo = 0;
			GLint uMVP = -1, uMV = -1, uTex = -1, uUseTex = -1, uTexEnv = -1, uAlphaFunc = -1, uAlphaRef = -1;
			GLint uFogMode = -1, uFogStart = -1, uFogEnd = -1, uFogDensity = -1, uFogColor = -1;
			std::string m_error;

			GLuint Compile(GLenum type, const char* src)
			{
				GLuint sh = glCreateShader(type);
				glShaderSource(sh, 1, &src, nullptr);
				glCompileShader(sh);
				GLint ok = 0;
				glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
				if (!ok)
				{
					char log[1024]; GLsizei n = 0; glGetShaderInfoLog(sh, sizeof log, &n, log);
					m_error = std::string(type == GL_VERTEX_SHADER ? "vertex: " : "fragment: ") + log;
					glDeleteShader(sh);
					return 0;
				}
				return sh;
			}
		};
	}
}
