#pragma once

#include <memory>
#include <vector>

#include "paint/ICommand.h"

namespace paint {

/// Gestor de historial de comandos: ejecuta, deshace y rehace.
/// El cliente (App) no lleva la contabilidad de la historia; este objeto sí.
class CommandManager {
public:
    /// Ejecuta el comando y lo apila. Cualquier acción nueva invalida la pila
    /// de "rehacer".
    void Execute(std::unique_ptr<ICommand> command);

    bool Undo();  // devuelve false si no había nada que deshacer
    bool Redo();  // devuelve false si no había nada que rehacer

    bool CanUndo() const;
    bool CanRedo() const;

private:
    std::vector<std::unique_ptr<ICommand>> done_;
    std::vector<std::unique_ptr<ICommand>> undone_;
};

}  // namespace paint
