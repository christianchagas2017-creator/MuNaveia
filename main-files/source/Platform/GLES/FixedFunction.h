#pragma once

// Emulacao do OpenGL de funcao fixa (o que o cliente usa) para backends sem ele,
// como OpenGL ES 3 (Android, iOS).
//
// O cliente usa so um subconjunto pequeno: modo imediato (glBegin/glVertex/glEnd),
// arrays de vertices, duas pilhas de matriz, cor atual, textura 2D com
// MODULATE/ADD, teste alfa e neblina. Aqui esse estado e mantido em C++ puro
// (sem incluir nenhum header de GL) e cada desenho e entregue a um Backend ja
// "achatado": vertices + um instantaneo do estado necessario ao shader.
//
// Header unico, sem dependencias alem da biblioteca padrao.

#include <vector>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <cstddef>

namespace Platform
{
	namespace FF
	{
		// Valores identicos aos do OpenGL. Nomes sem o prefixo GL_ para nao colidir
		// com os macros do gl_compat.h.
		enum : unsigned
		{
			kPoints = 0, kLines = 1, kLineLoop = 2, kLineStrip = 3, kTriangles = 4,
			kTriangleStrip = 5, kTriangleFan = 6, kQuads = 7, kQuadStrip = 8, kPolygon = 9,

			kModelView = 0x1700, kProjection = 0x1701, kTexture = 0x1702,

			kVertexArray = 0x8074, kNormalArray = 0x8075, kColorArray = 0x8076, kTexCoordArray = 0x8078,

			kTexture2D = 0x0DE1, kAlphaTest = 0x0BC0, kFog = 0x0B60,

			kFogMode = 0x0B65, kFogDensity = 0x0B62, kFogStart = 0x0B63, kFogEnd = 0x0B64, kFogColor = 0x0B66,
			kLinear = 0x2601, kExp = 0x0800, kExp2 = 0x0801,

			kTexEnv = 0x2300, kTexEnvMode = 0x2200,
			kModulate = 0x2100, kDecal = 0x2101, kReplace = 0x1E01, kAdd = 0x0104,

			kNever = 0x0200, kLess = 0x0201, kEqual = 0x0202, kLEqual = 0x0203,
			kGreater = 0x0204, kNotEqual = 0x0205, kGEqual = 0x0206, kAlways = 0x0207,

			kCurrentColor = 0x0B00, kModelViewMatrix = 0x0BA6, kProjectionMatrix = 0x0BA7,

			kUnsignedByte = 0x1401, kShort = 0x1402, kInt = 0x1404, kFloat = 0x1406, kDouble = 0x140A
		};

		// ---- Matrizes (coluna-principal, como o OpenGL: m[coluna*4 + linha]) ----
		struct Mat4
		{
			float m[16];

			static Mat4 Identity()
			{
				Mat4 r; std::memset(r.m, 0, sizeof r.m);
				r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
				return r;
			}

			// r = a * b
			static Mat4 Mul(const Mat4& a, const Mat4& b)
			{
				Mat4 r;
				for (int c = 0; c < 4; ++c)
					for (int row = 0; row < 4; ++row)
					{
						float s = 0.0f;
						for (int k = 0; k < 4; ++k) s += a.m[k * 4 + row] * b.m[c * 4 + k];
						r.m[c * 4 + row] = s;
					}
				return r;
			}
		};

		struct Vertex
		{
			float x, y, z;
			float r, g, b, a;
			float u, v;
		};

		// Tudo que o shader precisa para reproduzir o estado da funcao fixa.
		struct DrawState
		{
			Mat4  mvp;                 // projecao * modelview
			Mat4  modelView;           // para a distancia da neblina (z em espaco do olho)
			bool  texture2D = false;
			unsigned texEnv = kModulate;
			unsigned alphaFunc = 0;    // 0 = teste alfa desligado; senao kNever..kAlways
			float alphaRef = 0.0f;
			unsigned fogMode = 0;      // 0 = desligada; senao kLinear/kExp/kExp2
			float fogStart = 0.0f, fogEnd = 1.0f, fogDensity = 1.0f;
			float fogColor[4] = { 0, 0, 0, 0 };
		};

		struct DrawBatch
		{
			unsigned mode = kTriangles;   // ja valido em OpenGL ES: points, lines, line loop/strip, triangles, strip, fan
			std::vector<Vertex> vertices;
			DrawState state;
		};

		class Backend
		{
		public:
			virtual ~Backend() {}
			virtual void Draw(const DrawBatch& batch) = 0;
		};

		class Emulator
		{
		public:
			explicit Emulator(Backend* backend = nullptr) { SetBackend(backend); Reset(); }

			void SetBackend(Backend* b) { m_backend = b; }

			void Reset()
			{
				for (int i = 0; i < 3; ++i) { m_stack[i].clear(); m_stack[i].push_back(Mat4::Identity()); }
				m_mode = kModelView;
				m_color[0] = m_color[1] = m_color[2] = m_color[3] = 1.0f;
				m_u = m_v = 0.0f;
				m_inBegin = false;
				m_tex2D = m_alphaTest = m_fog = false;
				m_alphaFunc = kAlways; m_alphaRef = 0.0f;
				m_fogMode = kExp; m_fogDensity = 1.0f; m_fogStart = 0.0f; m_fogEnd = 1.0f;
				m_fogColor[0] = m_fogColor[1] = m_fogColor[2] = m_fogColor[3] = 0.0f;
				m_texEnv = kModulate;
				m_vertexArray = m_colorArray = m_texCoordArray = Array();
				errors = 0;
			}

			// Chamadas invalidas (Begin aninhado, End sem Begin...) so incrementam o contador.
			int errors = 0;

			// ---------------- Matrizes ----------------
			void MatrixMode(unsigned mode)
			{
				if (mode != kModelView && mode != kProjection && mode != kTexture) { ++errors; return; }
				m_mode = mode;
			}
			void LoadIdentity()             { Top() = Mat4::Identity(); }
			void LoadMatrix(const float* m) { std::memcpy(Top().m, m, sizeof(float) * 16); }
			void MultMatrix(const float* m) { Mat4 b; std::memcpy(b.m, m, sizeof b.m); Top() = Mat4::Mul(Top(), b); }
			void PushMatrix()
			{
				std::vector<Mat4>& s = m_stack[Index()];
				if (s.size() >= 64) { ++errors; return; }
				s.push_back(s.back());
			}
			void PopMatrix()
			{
				std::vector<Mat4>& s = m_stack[Index()];
				if (s.size() <= 1) { ++errors; return; }
				s.pop_back();
			}
			void Translate(float x, float y, float z)
			{
				Mat4 t = Mat4::Identity(); t.m[12] = x; t.m[13] = y; t.m[14] = z;
				Top() = Mat4::Mul(Top(), t);
			}
			void Scale(float x, float y, float z)
			{
				Mat4 s = Mat4::Identity(); s.m[0] = x; s.m[5] = y; s.m[10] = z;
				Top() = Mat4::Mul(Top(), s);
			}
			// angulo em graus, como glRotatef
			void Rotate(float angle, float x, float y, float z)
			{
				float len = std::sqrt(x * x + y * y + z * z);
				if (len == 0.0f) return;
				x /= len; y /= len; z /= len;
				const float rad = angle * 3.14159265358979323846f / 180.0f;
				const float c = std::cos(rad), s = std::sin(rad), t = 1.0f - c;
				Mat4 r = Mat4::Identity();
				r.m[0] = t * x * x + c;      r.m[4] = t * x * y - s * z;  r.m[8]  = t * x * z + s * y;
				r.m[1] = t * x * y + s * z;  r.m[5] = t * y * y + c;      r.m[9]  = t * y * z - s * x;
				r.m[2] = t * x * z - s * y;  r.m[6] = t * y * z + s * x;  r.m[10] = t * z * z + c;
				Top() = Mat4::Mul(Top(), r);
			}
			void Ortho(double l, double r, double b, double t, double n, double f)
			{
				Mat4 o = Mat4::Identity();
				o.m[0]  = (float)(2.0 / (r - l));
				o.m[5]  = (float)(2.0 / (t - b));
				o.m[10] = (float)(-2.0 / (f - n));
				o.m[12] = (float)(-(r + l) / (r - l));
				o.m[13] = (float)(-(t + b) / (t - b));
				o.m[14] = (float)(-(f + n) / (f - n));
				Top() = Mat4::Mul(Top(), o);
			}
			void Ortho2D(double l, double r, double b, double t) { Ortho(l, r, b, t, -1.0, 1.0); }
			// gluPerspective: fovy em graus
			void Perspective(double fovy, double aspect, double n, double f)
			{
				const double fc = 1.0 / std::tan(fovy * 3.14159265358979323846 / 360.0);
				Mat4 p; std::memset(p.m, 0, sizeof p.m);
				p.m[0]  = (float)(fc / aspect);
				p.m[5]  = (float)fc;
				p.m[10] = (float)((f + n) / (n - f));
				p.m[11] = -1.0f;
				p.m[14] = (float)(2.0 * f * n / (n - f));
				Top() = Mat4::Mul(Top(), p);
			}

			// ---------------- Cor / textura atuais ----------------
			void Color(float r, float g, float b, float a = 1.0f) { m_color[0] = r; m_color[1] = g; m_color[2] = b; m_color[3] = a; }
			void Color4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
			{
				Color(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
			}
			void TexCoord(float u, float v) { m_u = u; m_v = v; }

			// ---------------- Modo imediato ----------------
			void Begin(unsigned mode)
			{
				if (m_inBegin || mode > kPolygon) { ++errors; return; }
				m_inBegin = true; m_beginMode = mode; m_imm.clear();
			}
			void Vertex(float x, float y, float z = 0.0f)
			{
				if (!m_inBegin) { ++errors; return; }
				Platform::FF::Vertex v = { x, y, z, m_color[0], m_color[1], m_color[2], m_color[3], m_u, m_v };
				m_imm.push_back(v);
			}
			void End()
			{
				if (!m_inBegin) { ++errors; return; }
				m_inBegin = false;
				Submit(m_beginMode, m_imm);
				m_imm.clear();
			}

			// ---------------- Arrays de vertices (memoria do cliente) ----------------
			void EnableClientState(unsigned cap, bool on = true)
			{
				switch (cap)
				{
				case kVertexArray:   m_vertexArray.enabled = on; break;
				case kColorArray:    m_colorArray.enabled = on; break;
				case kTexCoordArray: m_texCoordArray.enabled = on; break;
				case kNormalArray:   break; // sem iluminacao: normais nao sao usadas
				default: ++errors;
				}
			}
			void DisableClientState(unsigned cap) { EnableClientState(cap, false); }

			void VertexPointer(int size, unsigned type, int stride, const void* p)   { SetArray(m_vertexArray, size, type, stride, p, 2, 4); }
			void ColorPointer(int size, unsigned type, int stride, const void* p)    { SetArray(m_colorArray, size, type, stride, p, 3, 4); }
			void TexCoordPointer(int size, unsigned type, int stride, const void* p) { SetArray(m_texCoordArray, size, type, stride, p, 1, 4); }

			void DrawArrays(unsigned mode, int first, int count)
			{
				if (m_inBegin || mode > kPolygon || first < 0 || count < 0) { ++errors; return; }
				if (!m_vertexArray.enabled || !m_vertexArray.ptr) return;

				std::vector<Platform::FF::Vertex> verts;
				verts.reserve((size_t)count);
				for (int i = first; i < first + count; ++i)
				{
					Platform::FF::Vertex v;
					float f[4];

					ReadAttrib(m_vertexArray, i, f);
					v.x = f[0]; v.y = f[1]; v.z = f[2];

					if (m_colorArray.enabled && m_colorArray.ptr)
					{
						ReadAttrib(m_colorArray, i, f);
						v.r = f[0]; v.g = f[1]; v.b = f[2]; v.a = f[3];
					}
					else { v.r = m_color[0]; v.g = m_color[1]; v.b = m_color[2]; v.a = m_color[3]; }

					if (m_texCoordArray.enabled && m_texCoordArray.ptr)
					{
						ReadAttrib(m_texCoordArray, i, f);
						v.u = f[0]; v.v = f[1];
					}
					else { v.u = m_u; v.v = m_v; }

					verts.push_back(v);
				}
				Submit(mode, verts);
			}

			// ---------------- Estado (glEnable, glAlphaFunc, glFog, glTexEnv) ----------------
			// Devolvem true se o estado e emulado aqui (nao deve ir ao driver).
			bool Enable(unsigned cap, bool on = true)
			{
				switch (cap)
				{
				case kTexture2D: m_tex2D = on; return true;
				case kAlphaTest: m_alphaTest = on; return true;
				case kFog:       m_fog = on; return true;
				default:         return false;
				}
			}
			bool Disable(unsigned cap) { return Enable(cap, false); }
			bool IsEnabled(unsigned cap) const
			{
				switch (cap) { case kTexture2D: return m_tex2D; case kAlphaTest: return m_alphaTest; case kFog: return m_fog; default: return false; }
			}

			void AlphaFunc(unsigned func, float ref)
			{
				if (func < kNever || func > kAlways) { ++errors; return; }
				m_alphaFunc = func; m_alphaRef = ref < 0.0f ? 0.0f : (ref > 1.0f ? 1.0f : ref);
			}

			void Fogf(unsigned pname, float v)
			{
				switch (pname)
				{
				case kFogMode:
					if (v == (float)kLinear || v == (float)kExp || v == (float)kExp2) m_fogMode = (unsigned)v; else ++errors;
					break;
				case kFogDensity: if (v >= 0.0f) m_fogDensity = v; else ++errors; break;
				case kFogStart:   m_fogStart = v; break;
				case kFogEnd:     m_fogEnd = v; break;
				default: ++errors;
				}
			}
			void Fogi(unsigned pname, int v) { Fogf(pname, (float)v); }
			void Fogfv(unsigned pname, const float* p)
			{
				if (pname == kFogColor) { for (int i = 0; i < 4; ++i) m_fogColor[i] = p[i] < 0 ? 0 : (p[i] > 1 ? 1 : p[i]); }
				else Fogf(pname, p[0]);
			}

			// glTexEnvf/i(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, modo)
			void TexEnv(unsigned target, unsigned pname, unsigned value)
			{
				if (target != kTexEnv || pname != kTexEnvMode) { ++errors; return; }
				if (value == kModulate || value == kAdd || value == kReplace || value == kDecal) m_texEnv = value; else ++errors;
			}

			// glGetFloatv de estados emulados. true = tratado.
			bool GetFloatv(unsigned pname, float* out) const
			{
				switch (pname)
				{
				case kCurrentColor:      std::memcpy(out, m_color, sizeof m_color); return true;
				case kModelViewMatrix:   std::memcpy(out, m_stack[0].back().m, sizeof(float) * 16); return true;
				case kProjectionMatrix:  std::memcpy(out, m_stack[1].back().m, sizeof(float) * 16); return true;
				default: return false;
				}
			}

			unsigned CurrentMatrixMode() const { return m_mode; }
			const Mat4& ModelView() const { return m_stack[0].back(); }
			const Mat4& Projection() const { return m_stack[1].back(); }

		private:
			struct Array
			{
				bool enabled = false;
				int size = 0;
				unsigned type = kFloat;
				int stride = 0;
				const void* ptr = nullptr;
			};

			std::vector<Mat4> m_stack[3];
			unsigned m_mode = kModelView;
			float m_color[4];
			float m_u = 0, m_v = 0;
			bool m_inBegin = false;
			unsigned m_beginMode = kTriangles;
			std::vector<Platform::FF::Vertex> m_imm;

			bool m_tex2D = false, m_alphaTest = false, m_fog = false;
			unsigned m_alphaFunc = kAlways; float m_alphaRef = 0.0f;
			unsigned m_fogMode = kExp; float m_fogDensity = 1.0f, m_fogStart = 0.0f, m_fogEnd = 1.0f, m_fogColor[4];
			unsigned m_texEnv = kModulate;

			Array m_vertexArray, m_colorArray, m_texCoordArray;
			Backend* m_backend = nullptr;

			int Index() const { return m_mode == kModelView ? 0 : (m_mode == kProjection ? 1 : 2); }
			Mat4& Top() { return m_stack[Index()].back(); }

			void SetArray(Array& a, int size, unsigned type, int stride, const void* p, int minSize, int maxSize)
			{
				if (size < minSize || size > maxSize || stride < 0) { ++errors; return; }
				if (type != kFloat && type != kDouble && type != kShort && type != kInt && type != kUnsignedByte) { ++errors; return; }
				a.size = size; a.type = type; a.stride = stride; a.ptr = p;
			}

			static size_t TypeSize(unsigned t)
			{
				switch (t) { case kUnsignedByte: return 1; case kShort: return 2; case kInt: return 4; case kDouble: return 8; default: return 4; }
			}

			// Le o elemento 'i' do array para 4 floats (o que falta: 0,0,0,1).
			// GL_UNSIGNED_BYTE e normalizado (0..255 -> 0..1), como em glColorPointer.
			static void ReadAttrib(const Array& a, int i, float out[4])
			{
				out[0] = out[1] = out[2] = 0.0f; out[3] = 1.0f;
				const size_t esz = TypeSize(a.type);
				const size_t stride = a.stride ? (size_t)a.stride : esz * (size_t)a.size;
				const unsigned char* p = (const unsigned char*)a.ptr + stride * (size_t)i;
				for (int c = 0; c < a.size; ++c)
				{
					const unsigned char* q = p + esz * (size_t)c;
					float v = 0.0f;
					switch (a.type)
					{
					case kFloat:        { float f;   std::memcpy(&f, q, 4); v = f; } break;
					case kDouble:       { double d;  std::memcpy(&d, q, 8); v = (float)d; } break;
					case kShort:        { int16_t s; std::memcpy(&s, q, 2); v = (float)s; } break;
					case kInt:          { int32_t n; std::memcpy(&n, q, 4); v = (float)n; } break;
					case kUnsignedByte: v = (*q) / 255.0f; break;
					}
					out[c] = v;
				}
			}

			DrawState Snapshot() const
			{
				DrawState s;
				s.modelView = m_stack[0].back();
				s.mvp = Mat4::Mul(m_stack[1].back(), m_stack[0].back());
				s.texture2D = m_tex2D;
				s.texEnv = m_texEnv;
				s.alphaFunc = m_alphaTest ? m_alphaFunc : 0;
				s.alphaRef = m_alphaRef;
				s.fogMode = m_fog ? m_fogMode : 0;
				s.fogStart = m_fogStart; s.fogEnd = m_fogEnd; s.fogDensity = m_fogDensity;
				std::memcpy(s.fogColor, m_fogColor, sizeof s.fogColor);
				return s;
			}

			// Converte o que o OpenGL ES nao tem (GL_QUADS, GL_QUAD_STRIP, GL_POLYGON) e
			// descarta primitivas incompletas, como o GL faz.
			void Submit(unsigned mode, const std::vector<Platform::FF::Vertex>& in)
			{
				if (!m_backend) return;
				const size_t n = in.size();
				DrawBatch b;
				b.state = Snapshot();

				switch (mode)
				{
				case kQuads:
				{
					const size_t quads = n / 4;
					if (!quads) return;
					b.mode = kTriangles;
					b.vertices.reserve(quads * 6);
					for (size_t q = 0; q < quads; ++q)
					{
						const Platform::FF::Vertex* v = &in[q * 4];
						b.vertices.push_back(v[0]); b.vertices.push_back(v[1]); b.vertices.push_back(v[2]);
						b.vertices.push_back(v[0]); b.vertices.push_back(v[2]); b.vertices.push_back(v[3]);
					}
					break;
				}
				case kQuadStrip:
				{
					if (n < 4) return;
					b.mode = kTriangleStrip;
					b.vertices.assign(in.begin(), in.begin() + (n & ~(size_t)1)); // numero par de vertices
					break;
				}
				case kPolygon:
					if (n < 3) return;
					b.mode = kTriangleFan; b.vertices = in; break;
				case kTriangles:
				{
					const size_t used = (n / 3) * 3;
					if (!used) return;
					b.mode = kTriangles; b.vertices.assign(in.begin(), in.begin() + used); break;
				}
				case kLines:
				{
					const size_t used = (n / 2) * 2;
					if (!used) return;
					b.mode = kLines; b.vertices.assign(in.begin(), in.begin() + used); break;
				}
				case kTriangleStrip:
				case kTriangleFan:
					if (n < 3) return;
					b.mode = mode; b.vertices = in; break;
				case kLineStrip:
				case kLineLoop:
					if (n < 2) return;
					b.mode = mode; b.vertices = in; break;
				default: // kPoints
					if (!n) return;
					b.mode = mode; b.vertices = in; break;
				}
				m_backend->Draw(b);
			}
		};
	}
}
