#pragma once

#include "stem/commands/TodoCommands.hpp"
#include "stomata/Logger.hpp"
#include "stomata/TodoRepository.hpp"

namespace planning::application {

// Todo 삭제. 태그는 Todo 에 내장되어 함께 제거된다(SQLite 어댑터에선 todo_tags cascade).
class DeleteTodoUseCase {
public:
    DeleteTodoUseCase(ports::TodoRepository& todos, ports::Logger& logger);

    // 대상이 없으면 std::out_of_range.
    void execute(const DeleteTodoCommand& cmd);

private:
    ports::TodoRepository& todos_;
    ports::Logger& logger_;
};

}  // namespace planning::application
