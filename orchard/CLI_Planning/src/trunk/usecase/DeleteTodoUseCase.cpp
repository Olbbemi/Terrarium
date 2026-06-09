#include "trunk/usecase/DeleteTodoUseCase.hpp"

#include <stdexcept>

namespace planning::application {

DeleteTodoUseCase::DeleteTodoUseCase(ports::TodoRepository& todos,
                                     toolshed::log::Logger& logger)
    : todos_(todos), logger_(logger) {}

void DeleteTodoUseCase::execute(const DeleteTodoCommand& cmd) {
    if (!todos_.findById(cmd.id)) {
        throw std::out_of_range("DeleteTodoUseCase: todo not found");
    }
    todos_.remove(cmd.id);
    logger_.audit("todo.delete", "deleted");
}

}  // namespace planning::application
