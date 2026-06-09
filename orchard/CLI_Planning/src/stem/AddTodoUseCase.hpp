#pragma once

#include "seed/IdGenerator.hpp"
#include "seed/Todo.hpp"
#include "stem/commands/TodoCommands.hpp"
#include "stomata/Logger.hpp"
#include "stomata/TodoRepository.hpp"

namespace planning::application {

class AddTodoUseCase {
public:
    AddTodoUseCase(ports::TodoRepository& todos, domain::IdGenerator& idGen,
                   ports::Logger& logger);

    // title 이 비어 있으면 std::invalid_argument(Todo 불변식).
    domain::Todo::Id execute(const AddTodoCommand& cmd);

private:
    ports::TodoRepository& todos_;
    domain::IdGenerator& idGen_;
    ports::Logger& logger_;
};

}  // namespace planning::application
