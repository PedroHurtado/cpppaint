#include "paint/ShapeFactory.h"

#include <sstream>
#include <stdexcept>
#include <utility>

namespace paint {

void ShapeFactory::Register(const std::string& name, const std::string& args,
                            const std::string& description, Builder builder) {
    entries_[name] = Entry{args, description, std::move(builder)};
}

bool ShapeFactory::Knows(const std::string& name) const {
    return entries_.find(name) != entries_.end();
}

void ShapeFactory::ForEachListed(
    const std::function<void(const std::string&, const std::string&)>& visit)
    const {
    for (const auto& entry : entries_) {  // std::map => ya ordenados
        const std::string& args = entry.second.args;
        visit(args.empty() ? entry.first : entry.first + " " + args,
              entry.second.description);
    }
}

std::unique_ptr<IShape> ShapeFactory::Create(const std::string& line) const {
    std::istringstream in(line);
    std::string name;
    in >> name;

    auto it = entries_.find(name);
    if (it == entries_.end())
        throw std::runtime_error("figura desconocida: " + name);

    return it->second.builder(in);
}

}  // namespace paint
