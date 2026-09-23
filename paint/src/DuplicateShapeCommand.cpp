#include "paint/DuplicateShapeCommand.h"

#include <memory>
#include <utility>

#include "paint/ICanvas.h"
#include "paint/IShape.h"

namespace paint {

DuplicateShapeCommand::DuplicateShapeCommand(ICanvas& canvas, std::size_t index)
    : canvas_(canvas), index_(index) {}

void DuplicateShapeCommand::Execute() {
    std::unique_ptr<IShape> copy = canvas_.At(index_).Clone();  // Prototype
    handle_ = copy.get();
    canvas_.Add(std::move(copy));
}

void DuplicateShapeCommand::Undo() {
    canvas_.Remove(handle_);
    handle_ = nullptr;
}

}  // namespace paint
