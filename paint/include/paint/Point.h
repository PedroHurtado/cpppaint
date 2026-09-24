#pragma once

namespace paint {

/// Posición de una figura en el lienzo (coordenadas enteras).
/// Es un tipo de valor sencillo: se copia libremente y no necesita .cpp.
struct Point {
    int x = 0;
    int y = 0;
};

/// Igualdad por valor. La pidieron los tests: sin ella ni EXPECT_EQ ni
/// EXPECT_CALL(shape, MoveTo(Point{7, 8})) saben comparar dos posiciones.
inline bool operator==(const Point& a, const Point& b) {
    return a.x == b.x && a.y == b.y;
}

inline bool operator!=(const Point& a, const Point& b) { return !(a == b); }

}  // namespace paint
