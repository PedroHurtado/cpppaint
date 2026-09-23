#include "paint/MoveShapeCommand.h"

#include "paint/ICanvas.h"
#include "paint/IShape.h"

namespace paint {

MoveShapeCommand::MoveShapeCommand(ICanvas& canvas, std::size_t index, Point target)
    : canvas_(canvas), index_(index), target_(target) {}

void MoveShapeCommand::Execute() {
    IShape& shape = canvas_.At(index_);  // valida el índice (lanza si no existe)
    handle_ = &shape;
    previous_ = shape.Position();        // guardamos el estado para deshacer
    shape.MoveTo(target_);
}

void MoveShapeCommand::Undo() {
    if (handle_) handle_->MoveTo(previous_);
}

}  // namespace paint
