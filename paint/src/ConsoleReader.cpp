#include "paint/ConsoleReader.h"

#include <iostream>

namespace paint {

bool ConsoleReader::ReadLine(std::string& line) {
    return static_cast<bool>(std::getline(std::cin, line));
}

}  // namespace paint
