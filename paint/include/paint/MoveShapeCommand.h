#pragma once

#include <cstddef>

#include "paint/ICommand.h"
#include "paint/Point.h"

namespace paint {

class ICanvas;
class IShape;

/// Mueve la figura de un índice a una posición destino. Guarda la posición
/// anterior para poder deshacer el movimiento.
class MoveShapeCommand : public ICommand {
public:
    MoveShapeCommand(ICanvas& canvas, std::size_t index, Point target);

    void Execute() override;
    void Undo() override;

private:
    ICanvas& canvas_;
    std::size_t index_;
    Point target_;
    Point previous_{};
    IShape* handle_ = nullptr;  // se resuelve en Execute y se reutiliza en Undo
};

}  // namespace paint
