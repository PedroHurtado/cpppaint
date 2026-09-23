#include "paint/Circle.h"

#include <string>

namespace paint {
namespace {
constexpr double kPi = 3.14159265358979;
}  // namespace

Circle::Circle(double radius, int color, Point position)
    : radius_(radius), color_(color), position_(position) {}

double Circle::Area() const { return kPi * radius_ * radius_; }

int Circle::Color() const { return color_; }

Point Circle::Position() const { return position_; }

void Circle::MoveTo(Point position) { position_ = position; }

std::unique_ptr<IShape> Circle::Clone() const {
    // Prototype: copia de sí misma con el constructor de copia generado.
    return std::make_unique<Circle>(*this);
}

std::string Circle::ToString() const {
    return "Circle | area: " + std::to_string(Area()) +
           " | color: " + std::to_string(color_) +
           " | pos: (" + std::to_string(position_.x) + ", " +
           std::to_string(position_.y) + ")";
}

}  // namespace paint
