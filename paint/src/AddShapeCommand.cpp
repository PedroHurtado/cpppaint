#include "paint/AddShapeCommand.h"

#include <utility>

#include "paint/ICanvas.h"

namespace paint {

AddShapeCommand::AddShapeCommand(ICanvas& canvas, std::unique_ptr<IShape> shape)
    : canvas_(canvas), shape_(std::move(shape)) {}

void AddShapeCommand::Execute() {
    handle_ = shape_.get();             // recordamos a quién añadimos
    canvas_.Add(std::move(shape_));     // cedemos la propiedad al lienzo
}

void AddShapeCommand::Undo() {
    shape_ = canvas_.Remove(handle_);   // recuperamos la propiedad
    handle_ = nullptr;
}

}  // namespace paint
