#pragma once

#include <vector>

#include "seed/Todo.hpp"
#include "stem/commands/TodoCommands.hpp"
#include "stomata/Logger.hpp"
#include "stomata/TodoRepository.hpp"

namespace planning::application {

// 활성 필터들의 AND 로 Todo 를 조회한다.
class ListTodosUseCase {
public:
    ListTodosUseCase(ports::TodoRepository& todos, ports::Logger& logger);

    std::vector<domain::Todo> execute(const ListTodosQuery& query);

private:
    ports::TodoRepository& todos_;
    ports::Logger& logger_;
};

}  // namespace planning::application
