#pragma once

#include <vector>

#include "trunk/domain/Todo.hpp"
#include "trunk/usecase/commands/TodoCommands.hpp"
#include "trunk/ports/Logger.hpp"
#include "trunk/ports/TodoRepository.hpp"

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
