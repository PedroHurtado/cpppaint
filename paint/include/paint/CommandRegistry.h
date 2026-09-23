#pragma once

#include <functional>
#include <iosfwd>
#include <map>
#include <string>

namespace paint {

class ICanvas;
class IWriter;
class CommandManager;
class ShapeFactory;
class CommandRegistry;

/// Qué debe hacer el REPL después de ejecutar una acción.
enum class Status { Continue, Quit };

/// Todo lo que una acción puede necesitar, empaquetado. Se lo pasa la App.
///
/// Gracias a esto las acciones son funciones libres sin estado: no son
/// métodos de App, así que registrar una nueva no obliga a tocar App.
struct AppContext {
    ICanvas& canvas;
    IWriter& writer;
    CommandManager& commands;
    const CommandRegistry& registry;  // para que "help" se liste solo
    const ShapeFactory& factory;      // idem con las figuras
};

/// Registro de verbos del REPL: misma filosofía que ShapeFactory, pero para
/// acciones en vez de figuras (Strategy + registro).
///
/// Antes, añadir un verbo obligaba a escribir otra rama en el if/else de
/// App::Run. Ahora es una llamada a Register() y App no se toca: eso es OCP.
class CommandRegistry {
public:
    /// Una acción recibe el contexto y el resto de la línea ya sin el verbo.
    using Action = std::function<Status(AppContext&, std::istream& args)>;

    /// `args` es la sintaxis de los parámetros ("<indice> <x> <y>") y
    /// `description` el texto de ayuda. Una `description` vacía marca el
    /// verbo como alias oculto: funciona, pero no se lista en la ayuda.
    void Register(const std::string& verb, const std::string& args,
                  const std::string& description, Action action);

    bool Knows(const std::string& verb) const;

    /// Ejecuta el verbo. Lanza std::runtime_error si no está registrado.
    Status Run(const std::string& verb, AppContext& context,
               std::istream& args) const;

    /// Recorre los verbos listables (los que tienen descripción), ordenados.
    void ForEachListed(
        const std::function<void(const std::string& usage,
                                 const std::string& description)>& visit) const;

private:
    struct Entry {
        std::string args;
        std::string description;
        Action action;
    };

    std::map<std::string, Entry> entries_;
};

}  // namespace paint
