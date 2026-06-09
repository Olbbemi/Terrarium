#include "trunk/usecase/MarkTodoDoneUseCase.hpp"

#include <stdexcept>

#include "trunk/domain/Todo.hpp"

namespace planning::application {

MarkTodoDoneUseCase::MarkTodoDoneUseCase(ports::TodoRepository& todos,
                                         ports::Logger& logger)
    : todos_(todos), logger_(logger) {}

void MarkTodoDoneUseCase::execute(const MarkTodoDoneCommand& cmd) {
    auto found = todos_.findById(cmd.id);
    if (!found) {
        throw std::out_of_range("MarkTodoDoneUseCase: todo not found");
    }
    domain::Todo todo = *found;
    todo.markDone();  // 이미 완료여도 멱등
    todos_.update(todo);
    logger_.audit("todo.done", "marked done");
}

}  // namespace planning::application
