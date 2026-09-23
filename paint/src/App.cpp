#include "paint/App.h"

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "paint/AddShapeCommand.h"
#include "paint/CommandRegistration.h"
#include "paint/ICanvas.h"
#include "paint/IReader.h"
#include "paint/IWriter.h"
#include "paint/ShapeRegistration.h"

namespace paint {

App::App(ICanvas& canvas, IReader& reader, IWriter& writer)
    : canvas_(canvas), reader_(reader), writer_(writer) {
    RegisterBuiltinShapes(factory_);
    RegisterBuiltinCommands(registry_);
}

void App::Run() {
    AppContext context{canvas_, writer_, commands_, registry_, factory_};

    std::istringstream empty;
    registry_.Run("help", context, empty);

    std::string line;
    while (reader_.ReadLine(line)) {
        std::istringstream args(line);
        std::string verb;
        if (!(args >> verb)) continue;  // línea en blanco

        // Tres ramas, y siempre tres: un verbo registrado, una figura
        // conocida por la fábrica, o nada. Añadir verbos o figuras no
        // añade ramas aquí (OCP).
        try {
            if (registry_.Knows(verb)) {
                if (registry_.Run(verb, context, args) == Status::Quit) break;
            } else if (factory_.Knows(verb)) {
                // La fábrica parsea la línea completa y construye la figura.
                commands_.Execute(std::make_unique<AddShapeCommand>(
                    canvas_, factory_.Create(line)));
                canvas_.Print(writer_);
            } else {
                writer_.Write("comando desconocido: " + verb);
            }
        } catch (const std::exception& ex) {
            writer_.Write(std::string("error: ") + ex.what());
        }
    }
}

}  // namespace paint
