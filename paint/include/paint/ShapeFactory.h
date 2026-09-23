#pragma once

#include <functional>
#include <istream>
#include <map>
#include <memory>
#include <string>

#include "paint/IShape.h"

namespace paint {

/// Factory con registro. Añadir una figura nueva consiste en registrar un
/// "builder" (OCP): no se toca esta clase.
///
/// Nota de diseño: la fábrica es instanciable e inyectable (la app posee la
/// suya). Evitamos a propósito la fábrica 100% estática, que de facto es un
/// Singleton oculto y dificulta los tests (ver doc/day-03/03_factory.md §8).
class ShapeFactory {
public:
    /// Un builder recibe el resto de la línea ya tokenizada y construye la
    /// figura leyendo de ella sus parámetros.
    using Builder = std::function<std::unique_ptr<IShape>(std::istream&)>;

    /// `args` es la sintaxis de los parametros ("<radio> <color> <x> <y>")
    /// y `description` el texto de ayuda. La ayuda se genera de aqui: si
    /// registras una figura con otros parametros, el help lo refleja solo.
    void Register(const std::string& name, const std::string& args,
                  const std::string& description, Builder builder);
    bool Knows(const std::string& name) const;

    /// Recorre las figuras registradas, ordenadas. Mismo contrato que
    /// CommandRegistry::ForEachListed: la ayuda las trata igual.
    void ForEachListed(
        const std::function<void(const std::string& usage,
                                 const std::string& description)>& visit)
        const;

    /// Crea una figura a partir de una línea con formato:
    ///   "<nombre> <param1> <param2> ..."
    /// Lanza std::runtime_error si el nombre no está registrado.
    std::unique_ptr<IShape> Create(const std::string& line) const;

private:
    struct Entry {
        std::string args;
        std::string description;
        Builder builder;
    };

    std::map<std::string, Entry> entries_;
};

}  // namespace paint
