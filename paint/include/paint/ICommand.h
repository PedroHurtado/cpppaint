#pragma once

namespace paint {

/// Command: encapsula una acción como objeto, con capacidad de deshacerse.
/// Interfaz mínima (ISP): solo ejecutar y deshacer.
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
};

}  // namespace paint
