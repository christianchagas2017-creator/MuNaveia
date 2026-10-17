#pragma once

// Acesso a arquivos portavel para o cliente.
//
// Problemas que resolve fora do Windows (Android, iOS, Linux):
//  1. O codigo usa caminhos no estilo Windows ("Data\\Custom\\x.ozj")  -> '\\' vira '/'.
//  2. O sistema de arquivos diferencia maiusculas de minusculas, mas os assets
//     do jogo foram nomeados no Windows ("Hud.OZJ" aberto como "hud.ozj") ->
//     cada componente do caminho e resolvido sem diferenciar maiusculas.
//  3. Em mobile o diretorio de trabalho nao e a pasta do jogo -> SetRootDir().
//
// Header unico (sem .cpp) para nao exigir mudancas em Main.vcxproj.
// No Windows as funcoes apenas repassam para o fopen nativo.

#include <cstdio>
#include <string>
#include <mutex>
#include <unordered_map>
#include "PlatformPath.h"

#ifndef _WIN32
#include <filesystem>
#include <cctype>
#include <vector>
#endif

namespace Platform
{
#ifdef _WIN32

	inline void SetRootDir(const std::string&) {}
	inline void InvalidateFileCache() {}
	inline std::string ResolvePath(const std::string& path) { return path; }
	inline FILE* FOpen(const char* path, const char* mode) { return (std::fopen)(path, mode); }

#else

	namespace detail
	{
		struct FileState
		{
			std::mutex mtx;
			std::string root;
			// diretorio -> (nome em minusculas -> nome real). Ausente do mapa = ainda nao listado.
			std::unordered_map<std::string, std::unordered_map<std::string, std::string>> dirs;
			// diretorios que ja sabemos que nao existem
			std::unordered_map<std::string, bool> missing;
		};

		inline FileState& State()
		{
			static FileState s;
			return s;
		}

		inline std::string Lower(const std::string& s)
		{
			std::string o(s);
			for (char& c : o) c = (char)std::tolower((unsigned char)c);
			return o;
		}

		inline std::string Join(const std::string& a, const std::string& b)
		{
			if (a.empty()) return b;
			if (a.back() == '/') return a + b;
			return a + "/" + b;
		}

		// Chamar com o mutex travado. Devolve o nome real de 'name' dentro de 'dir',
		// ou "" se o diretorio nao existe / nao ha entrada correspondente.
		inline std::string Lookup(FileState& st, const std::string& dir, const std::string& name)
		{
			auto it = st.dirs.find(dir);
			if (it == st.dirs.end())
			{
				if (st.missing.count(dir)) return std::string();

				std::error_code ec;
				std::filesystem::directory_iterator di(dir.empty() ? std::string(".") : dir, ec), end;
				if (ec)
				{
					st.missing[dir] = true;
					return std::string();
				}
				std::unordered_map<std::string, std::string> entries;
				std::vector<std::string> names;
				for (; di != end; di.increment(ec))
				{
					if (ec) break;
					names.push_back(di->path().filename().string());
				}
				// 1a passada: chave em minusculas (em colisao A.txt/a.txt a primeira vence).
				for (const std::string& real : names) entries.emplace(Lower(real), real);
				// 2a passada: chave exata sobrescreve, entao o nome escrito igual ao do disco sempre vence.
				for (const std::string& real : names) entries[real] = real;
				it = st.dirs.emplace(dir, std::move(entries)).first;
			}

			auto exact = it->second.find(name);
			if (exact != it->second.end()) return exact->second;
			auto ci = it->second.find(Lower(name));
			if (ci != it->second.end()) return ci->second;
			return std::string();
		}

		inline std::string ParentOf(const std::string& p)
		{
			size_t pos = p.find_last_of('/');
			if (pos == std::string::npos) return std::string();
			if (pos == 0) return "/";
			return p.substr(0, pos);
		}
	}

	// Define a pasta base para caminhos relativos (ex.: pasta de dados do app no Android/iOS).
	inline void SetRootDir(const std::string& root)
	{
		auto& st = detail::State();
		std::lock_guard<std::mutex> lock(st.mtx);
		st.root = NormalizePath(root);
		st.dirs.clear();
		st.missing.clear();
	}

	// Esquece as listagens em cache (use se arquivos forem criados/removidos por fora).
	inline void InvalidateFileCache()
	{
		auto& st = detail::State();
		std::lock_guard<std::mutex> lock(st.mtx);
		st.dirs.clear();
		st.missing.clear();
	}

	// Normaliza separadores e acerta maiusculas/minusculas de cada componente que existir.
	// Componentes que nao existem (arquivo a ser criado) ficam como foram escritos.
	inline std::string ResolvePath(const std::string& path)
	{
		using namespace detail;
		std::string norm = NormalizePath(path);
		if (norm.empty()) return norm;

		auto& st = State();
		std::lock_guard<std::mutex> lock(st.mtx);

		const bool absolute = (norm[0] == '/');
		std::string cur = absolute ? std::string("/") : st.root;

		size_t i = 0;
		bool lost = false; // true depois do primeiro componente inexistente
		while (i <= norm.size())
		{
			size_t j = norm.find('/', i);
			if (j == std::string::npos) j = norm.size();
			std::string comp = norm.substr(i, j - i);
			i = j + 1;

			if (comp.empty() || comp == ".") continue;

			if (lost || comp == "..")
			{
				cur = Join(cur, comp);
				continue;
			}

			std::string real = Lookup(st, cur, comp);
			if (real.empty())
			{
				lost = true;
				real = comp;
			}
			cur = Join(cur, real);
		}
		return cur;
	}

	inline FILE* FOpen(const char* path, const char* mode)
	{
		if (!path || !mode) return nullptr;

		std::string resolved = ResolvePath(path);
		FILE* f = (std::fopen)(resolved.c_str(), mode);

		// Se criamos/truncamos um arquivo, a listagem do diretorio ficou velha.
		if (f && (std::string(mode).find_first_of("wa+") != std::string::npos))
		{
			auto& st = detail::State();
			std::lock_guard<std::mutex> lock(st.mtx);
			std::string parent = detail::ParentOf(resolved);
			st.dirs.erase(parent);
			st.missing.erase(parent);
		}
		return f;
	}

#endif // _WIN32
}
