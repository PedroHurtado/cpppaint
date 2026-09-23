#include "paint/CommandRegistry.h"

#include <stdexcept>
#include <utility>

namespace paint {

void CommandRegistry::Register(const std::string& verb, const std::string& args,
                               const std::string& description, Action action) {
    entries_[verb] = Entry{args, description, std::move(action)};
}

bool CommandRegistry::Knows(const std::string& verb) const {
    return entries_.find(verb) != entries_.end();
}

Status CommandRegistry::Run(const std::string& verb, AppContext& context,
                            std::istream& args) const {
    auto it = entries_.find(verb);
    if (it == entries_.end())
        throw std::runtime_error("comando desconocido: " + verb);

    return it->second.action(context, args);
}

void CommandRegistry::ForEachListed(
    const std::function<void(const std::string&, const std::string&)>& visit)
    const {
    for (const auto& entry : entries_) {
        if (entry.second.description.empty()) continue;  // alias oculto
        const std::string& args = entry.second.args;
        visit(args.empty() ? entry.first : entry.first + " " + args,
              entry.second.description);
    }
}

}  // namespace paint
