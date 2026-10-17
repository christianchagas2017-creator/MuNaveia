#pragma once

// Utilitarios de caminho. O cliente grava caminhos no estilo Windows
// ("Data\\Custom\\Interface\\x.ozj"); em Android/iOS/Linux o separador e '/'
// e o sistema de arquivos diferencia maiusculas de minusculas.

#include <string>

namespace Platform
{
	// Troca '\\' por '/' (no Windows devolve o caminho sem alterar).
	inline std::string NormalizePath(const std::string& path)
	{
#ifdef _WIN32
		return path;
#else
		std::string out(path);
		for (char& c : out)
			if (c == '\\') c = '/';
		return out;
#endif
	}
}
