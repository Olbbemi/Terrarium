#pragma once

#include "trunk/domain/IdGenerator.hpp"
#include "trunk/domain/Todo.hpp"
#include "trunk/usecase/commands/TodoCommands.hpp"
#include "toolshed/log/Logger.hpp"
#include "trunk/ports/TodoRepository.hpp"

namespace planning::application {

class AddTodoUseCase {
public:
    AddTodoUseCase(ports::TodoRepository& todos, domain::IdGenerator& idGen,
                   toolshed::log::Logger& logger);

    // title 이 비어 있으면 std::invalid_argument(Todo 불변식).
    domain::Todo::Id execute(const AddTodoCommand& cmd);

private:
    ports::TodoRepository& todos_;
    domain::IdGenerator& idGen_;
    toolshed::log::Logger& logger_;
};

}  // namespace planning::application
