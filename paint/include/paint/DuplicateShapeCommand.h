#pragma once

#include <cstddef>

#include "paint/ICommand.h"

namespace paint {

class ICanvas;
class IShape;

/// Duplica la figura de un índice clonándola (Prototype) y añadiendo la copia.
/// Combina dos patrones: Prototype (clone) dentro de un Command (reversible).
class DuplicateShapeCommand : public ICommand {
public:
    DuplicateShapeCommand(ICanvas& canvas, std::size_t index);

    void Execute() override;
    void Undo() override;

private:
    ICanvas& canvas_;
    std::size_t index_;
    IShape* handle_ = nullptr;  // localiza la copia para deshacer
};

}  // namespace paint
