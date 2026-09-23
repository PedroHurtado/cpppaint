#include "paint/ConsoleWriter.h"

#include <iostream>

namespace paint {

void ConsoleWriter::Write(const std::string& line) { std::cout << line << '\n'; }

}  // namespace paint
