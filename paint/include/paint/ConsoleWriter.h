#pragma once

#include "paint/IWriter.h"

namespace paint {

/// Implementación concreta del puerto de salida: escribe en std::cout.
class ConsoleWriter : public IWriter {
public:
    void Write(const std::string& line) override;
};

}  // namespace paint
