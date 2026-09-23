#pragma once

namespace paint {

/// Posición de una figura en el lienzo (coordenadas enteras).
/// Es un tipo de valor sencillo: se copia libremente y no necesita .cpp.
struct Point {
    int x = 0;
    int y = 0;
};

}  // namespace paint
