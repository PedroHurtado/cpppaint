#include "paint/ShapeRegistration.h"

#include <istream>
#include <memory>

#include "paint/Circle.h"
#include "paint/Point.h"
#include "paint/ShapeFactory.h"
#include "paint/Square.h"

namespace paint {

void RegisterBuiltinShapes(ShapeFactory& factory) {
    factory.Register(
        "circle", "<radio> <color> <x> <y>", "anade un circulo",
        [](std::istream& in) -> std::unique_ptr<IShape> {
            double radius = 0;
            int color = 0;
            Point position;
            in >> radius >> color >> position.x >> position.y;
            return std::make_unique<Circle>(radius, color, position);
        });

    factory.Register(
        "square", "<lado> <color> <x> <y>", "anade un cuadrado",
        [](std::istream& in) -> std::unique_ptr<IShape> {
            double side = 0;
            int color = 0;
            Point position;
            in >> side >> color >> position.x >> position.y;
            return std::make_unique<Square>(side, color, position);
        });

    // Añadir una figura nueva (p. ej. "triangle") es otra llamada a Register
    // aquí o en otro .cpp. ShapeFactory no se toca, y la ayuda la recoge sola
    // con su sintaxis propia: eso es OCP.
}

}  // namespace paint
