#include "paint/Square.h"

#include <string>

namespace paint {

Square::Square(double side, int color, Point position)
    : side_(side), color_(color), position_(position) {}

double Square::Area() const { return side_ * side_; }

int Square::Color() const { return color_; }

Point Square::Position() const { return position_; }

void Square::MoveTo(Point position) { position_ = position; }

std::unique_ptr<IShape> Square::Clone() const {
    // Prototype: copia de sí misma con el constructor de copia generado.
    return std::make_unique<Square>(*this);
}

std::string Square::ToString() const {
    return "Square | area: " + std::to_string(Area()) +
           " | color: " + std::to_string(color_) +
           " | pos: (" + std::to_string(position_.x) + ", " +
           std::to_string(position_.y) + ")";
}

}  // namespace paint
