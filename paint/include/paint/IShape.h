#pragma once

#include <memory>
#include <string>

#include "paint/Point.h"

namespace paint {

/// Abstracción de figura. Los clientes (lienzo, comandos, app) dependen de
/// esta interfaz y nunca de un tipo concreto (DIP).
class IShape {
public:
    virtual ~IShape() = default;

    virtual double Area() const = 0;
    virtual int Color() const = 0;

    virtual Point Position() const = 0;
    virtual void MoveTo(Point position) = 0;

    /// Prototype: cada figura sabe fabricar una copia de sí misma sin que el
    /// cliente tenga que averiguar su tipo concreto.
    virtual std::unique_ptr<IShape> Clone() const = 0;

    /// Representación textual de la figura. La figura describe su contenido,
    /// pero no decide dónde se escribe (eso es trabajo del IWriter).
    virtual std::string ToString() const = 0;
};

}  // namespace paint
