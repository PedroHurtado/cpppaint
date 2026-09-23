#pragma once

#include "paint/CommandManager.h"
#include "paint/CommandRegistry.h"
#include "paint/ShapeFactory.h"

namespace paint {

class ICanvas;
class IReader;
class IWriter;

/// Bucle de lectura/ejecución (REPL). Traduce cada línea que teclea el usuario
/// en una acción registrada y la ejecuta.
///
/// Depende solo de abstracciones: el lienzo (ICanvas), la entrada (IReader) y
/// la salida (IWriter) entran por constructor, así que la App es testeable con
/// dobles sin tocar consola ni Singleton.
class App {
public:
    App(ICanvas& canvas, IReader& reader, IWriter& writer);

    /// Lee líneas del IReader hasta EOF o "exit".
    void Run();

    /// Puntos de extensión (OCP): añadir un verbo o una figura no obliga a
    /// modificar esta clase.
    CommandRegistry& Registry() { return registry_; }
    ShapeFactory& Factory() { return factory_; }

private:
    ICanvas& canvas_;
    IReader& reader_;
    IWriter& writer_;
    ShapeFactory factory_;
    CommandManager commands_;
    CommandRegistry registry_;
};

}  // namespace paint
