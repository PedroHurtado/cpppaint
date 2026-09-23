#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "paint/ICanvas.h"
#include "paint/IShape.h"
#include "paint/IWriter.h"

namespace paint {

/// Singleton de Meyers: en el dominio del Paint solo existe un lienzo activo.
/// Las cuatro defensas (ctor privado, copia/movimiento borrados, dtor privado)
/// garantizan que nadie pueda crear un segundo lienzo.
///
/// Recordatorio honesto: Singleton acopla y complica los tests (ver
/// doc/day-03/01_singleton.md §7). Está aquí porque el patrón hay que
/// enseñarlo; en una aplicación real el composition root crearía el único
/// lienzo y repartiría la referencia, que es justo lo que ya hace main().
/// De hecho, en cuanto existe ICanvas el Singleton deja de aportar nada.
///
/// Instance() devuelve ICanvas&, no Canvas&: nadie fuera de aquí necesita
/// el tipo concreto.
class Canvas : public ICanvas {
public:
    static ICanvas& Instance();

    void Add(std::unique_ptr<IShape> shape) override;
    std::unique_ptr<IShape> Remove(IShape* handle) override;
    IShape& At(std::size_t index) override;
    std::size_t Count() const override;
    void Clear() override;
    void Print(IWriter& writer) const override;

    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;
    Canvas(Canvas&&) = delete;
    Canvas& operator=(Canvas&&) = delete;

private:
    Canvas() = default;
    ~Canvas() = default;

    std::vector<std::unique_ptr<IShape>> shapes_;
};

}  // namespace paint
