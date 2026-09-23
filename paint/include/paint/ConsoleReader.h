#pragma once

#include <string>

#include "paint/IReader.h"

namespace paint {

/// Implementación concreta del puerto de entrada: lee de std::cin.
class ConsoleReader : public IReader {
public:
    bool ReadLine(std::string& line) override;
};

}  // namespace paint
