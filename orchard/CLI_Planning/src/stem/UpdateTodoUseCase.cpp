#include "stem/UpdateTodoUseCase.hpp"

#include <stdexcept>

#include "seed/Todo.hpp"

namespace planning::application {

UpdateTodoUseCase::UpdateTodoUseCase(ports::TodoRepository& todos,
                                     ports::Logger& logger)
    : todos_(todos), logger_(logger) {}

void UpdateTodoUseCase::execute(const UpdateTodoCommand& cmd) {
    auto found = todos_.findById(cmd.id);
    if (!found) {
        throw std::out_of_range("UpdateTodoUseCase: todo not found");
    }
    domain::Todo todo = *found;

    if (cmd.title) todo.rename(*cmd.title);
    if (cmd.priority) todo.updatePriority(*cmd.priority);
    if (cmd.due) todo.setDueDate(*cmd.due);
    if (cmd.tags) {
        const auto current = todo.tags();  // 스냅샷 복사 후 전체 교체
        for (const auto& t : current) todo.removeTag(t);
        for (const auto& t : *cmd.tags) todo.addTag(t);
    }

    todos_.update(todo);
    logger_.audit("todo.update", "updated");
}

}  // namespace planning::application
