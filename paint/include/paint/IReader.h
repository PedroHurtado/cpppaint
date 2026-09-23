#pragma once

#include <string>

namespace paint {

/// Puerto de entrada (DIP): el REPL no sabe de dónde vienen las líneas
/// (consola, fichero, string de test...). Solo conoce esta abstracción.
///
/// Nótese lo estrecha que es comparada con std::istream (ISP): la App solo
/// necesita "dame la siguiente línea", no formateo, ni locales, ni seekg.
class IReader {
public:
    virtual ~IReader() = default;

    /// Lee la siguiente línea en `line`. Devuelve false cuando no queda
    /// entrada (EOF), igual que std::getline en un if.
    virtual bool ReadLine(std::string& line) = 0;
};

}  // namespace paint
