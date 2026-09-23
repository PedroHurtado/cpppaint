#pragma once

#include <memory>

#include "paint/ICommand.h"
#include "paint/IShape.h"

namespace paint {

class ICanvas;

/// Añade una figura al lienzo. Al deshacer, la retira y recupera su propiedad
/// para poder volver a añadirla en un eventual "redo".
class AddShapeCommand : public ICommand {
public:
    AddShapeCommand(ICanvas& canvas, std::unique_ptr<IShape> shape);

    void Execute() override;
    void Undo() override;

private:
    ICanvas& canvas_;
    std::unique_ptr<IShape> shape_;  // la posee mientras la acción está deshecha
    IShape* handle_ = nullptr;       // la localiza mientras la acción está hecha
};

}  // namespace paint
