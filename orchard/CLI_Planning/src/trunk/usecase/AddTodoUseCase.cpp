#include "trunk/usecase/AddTodoUseCase.hpp"

namespace planning::application {

AddTodoUseCase::AddTodoUseCase(ports::TodoRepository& todos,
                               domain::IdGenerator& idGen, toolshed::log::Logger& logger)
    : todos_(todos), idGen_(idGen), logger_(logger) {}

domain::Todo::Id AddTodoUseCase::execute(const AddTodoCommand& cmd) {
    const auto id = idGen_.next();
    domain::Todo todo(id, cmd.title, cmd.priority, cmd.tags, cmd.due);
    todos_.save(todo);
    logger_.audit("todo.add", "added");
    return id;
}

}  // namespace planning::application
