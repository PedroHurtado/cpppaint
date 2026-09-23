#pragma once

#include <cstddef>
#include <memory>

#include "paint/IShape.h"

namespace paint {

class IWriter;

/// Abstracción del lienzo. Los comandos dependen de esta interfaz, nunca del
/// Canvas concreto (DIP): así se pueden probar contra un lienzo falso sin
/// arrastrar el Singleton a los tests.
///
/// OJO con el destructor: es `protected` y **no** virtual, a propósito.
/// Si fuera `public virtual` (lo habitual), `delete &canvas` sobre la
/// referencia que devuelve Canvas::Instance() compilaría sin un solo warning
/// —el acceso al destructor se comprueba sobre el tipo estático, ICanvas—
/// y destruiría el Singleton, que es un objeto estático: UB y crash.
/// El `~Canvas()` privado no protege de nada ahí, porque la llamada es
/// virtual y en tiempo de ejecución ya no hay control de acceso.
/// Con el destructor protegido, ese `delete` es error de compilación.
///
/// El precio: nadie puede poseer un ICanvas por unique_ptr. Aquí es
/// deliberado —el lienzo siempre se maneja por referencia, nunca se posee—.
class ICanvas {
public:
    virtual void Add(std::unique_ptr<IShape> shape) = 0;

    /// Extrae la figura apuntada por `handle` y devuelve su propiedad.
    virtual std::unique_ptr<IShape> Remove(IShape* handle) = 0;

    /// Acceso por índice, validado. Lanza std::out_of_range si no existe.
    virtual IShape& At(std::size_t index) = 0;

    virtual std::size_t Count() const = 0;
    virtual void Clear() = 0;
    virtual void Print(IWriter& writer) const = 0;

protected:
    ~ICanvas() = default;  // ver el comentario de arriba
};

}  // namespace paint
