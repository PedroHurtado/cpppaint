#include "paint/CommandRegistration.h"

#include <cstddef>
#include <istream>
#include <memory>
#include <string>

#include "paint/CommandManager.h"
#include "paint/CommandRegistry.h"
#include "paint/DuplicateShapeCommand.h"
#include "paint/ICanvas.h"
#include "paint/IWriter.h"
#include "paint/MoveShapeCommand.h"
#include "paint/Point.h"
#include "paint/ShapeFactory.h"

namespace paint {
namespace {

constexpr std::size_t kUsageWidth = 33;  // columna donde empieza la descripcion

std::string Pad(const std::string& text) {
    return text.size() >= kUsageWidth
               ? text + " "
               : text + std::string(kUsageWidth - text.size(), ' ');
}

Status Help(AppContext& context, std::istream&) {
    context.writer.Write("Comandos disponibles:");

    // La ayuda no tiene el texto escrito a mano: se lo pregunta al registro y
    // a la fabrica. Registrar un verbo o una figura la actualiza sola.
    context.registry.ForEachListed(
        [&context](const std::string& usage, const std::string& description) {
            context.writer.Write("  " + Pad(usage) + description);
        });

    context.factory.ForEachListed(
        [&context](const std::string& usage, const std::string& description) {
            context.writer.Write("  " + Pad(usage) + description);
        });

    return Status::Continue;
}

}  // namespace

void RegisterBuiltinCommands(CommandRegistry& registry) {
    registry.Register("help", "", "muestra esta ayuda", Help);

    registry.Register("print", "", "lista el lienzo",
                      [](AppContext& context, std::istream&) {
                          context.canvas.Print(context.writer);
                          return Status::Continue;
                      });

    registry.Register("undo", "", "deshace la ultima accion",
                      [](AppContext& context, std::istream&) {
                          context.writer.Write(context.commands.Undo()
                                                   ? "deshecho"
                                                   : "nada que deshacer");
                          return Status::Continue;
                      });

    registry.Register("redo", "", "rehace la ultima accion deshecha",
                      [](AppContext& context, std::istream&) {
                          context.writer.Write(context.commands.Redo()
                                                   ? "rehecho"
                                                   : "nada que rehacer");
                          return Status::Continue;
                      });

    registry.Register(
        "move", "<indice> <x> <y>", "mueve una figura",
        [](AppContext& context, std::istream& args) {
            std::size_t index = 0;
            Point target;
            if (!(args >> index >> target.x >> target.y)) {
                context.writer.Write("uso: move <indice> <x> <y>");
                return Status::Continue;
            }
            context.commands.Execute(std::make_unique<MoveShapeCommand>(
                context.canvas, index, target));
            context.canvas.Print(context.writer);
            return Status::Continue;
        });

    registry.Register(
        "duplicate", "<indice>", "duplica una figura (Prototype)",
        [](AppContext& context, std::istream& args) {
            std::size_t index = 0;
            if (!(args >> index)) {
                context.writer.Write("uso: duplicate <indice>");
                return Status::Continue;
            }
            context.commands.Execute(
                std::make_unique<DuplicateShapeCommand>(context.canvas, index));
            context.canvas.Print(context.writer);
            return Status::Continue;
        });

    registry.Register("exit", "", "termina",
                      [](AppContext&, std::istream&) { return Status::Quit; });

    // Alias: descripcion vacia => funcionan pero no ensucian la ayuda.
    registry.Register("list", "", "",
                      [](AppContext& context, std::istream&) {
                          context.canvas.Print(context.writer);
                          return Status::Continue;
                      });

    registry.Register("quit", "", "",
                      [](AppContext&, std::istream&) { return Status::Quit; });

    // Anadir un verbo nuevo es otra llamada a Register aqui o en otro .cpp.
    // App::Run no se toca: eso es OCP.
}

}  // namespace paint
