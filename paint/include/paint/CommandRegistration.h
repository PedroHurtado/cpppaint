#pragma once

namespace paint {

class CommandRegistry;

/// Registra los verbos "de serie" del REPL. Añadir uno nuevo es una línea más
/// aquí (o en otro fichero), sin tocar App ni CommandRegistry (OCP).
/// Es el gemelo de RegisterBuiltinShapes para las acciones.
void RegisterBuiltinCommands(CommandRegistry& registry);

}  // namespace paint
