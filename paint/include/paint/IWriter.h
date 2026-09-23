#pragma once

#include <string>

namespace paint {

/// Puerto de salida (DIP): el código que imprime no sabe a dónde van las
/// líneas (consola, fichero, test...). Solo conoce esta abstracción.
/// Simétrico de IReader.
class IWriter {
public:
    virtual ~IWriter() = default;
    virtual void Write(const std::string& line) = 0;
};

}  // namespace paint
