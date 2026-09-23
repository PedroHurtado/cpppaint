#include "paint/CommandManager.h"

#include <utility>

namespace paint {

void CommandManager::Execute(std::unique_ptr<ICommand> command) {
    command->Execute();
    done_.push_back(std::move(command));
    undone_.clear();  // una acción nueva invalida el "rehacer"
}

bool CommandManager::Undo() {
    if (done_.empty()) return false;
    auto command = std::move(done_.back());
    done_.pop_back();
    command->Undo();
    undone_.push_back(std::move(command));
    return true;
}

bool CommandManager::Redo() {
    if (undone_.empty()) return false;
    auto command = std::move(undone_.back());
    undone_.pop_back();
    command->Execute();
    done_.push_back(std::move(command));
    return true;
}

bool CommandManager::CanUndo() const { return !done_.empty(); }

bool CommandManager::CanRedo() const { return !undone_.empty(); }

}  // namespace paint
