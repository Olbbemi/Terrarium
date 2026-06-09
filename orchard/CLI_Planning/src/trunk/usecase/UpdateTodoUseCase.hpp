#pragma once

#include "trunk/usecase/commands/TodoCommands.hpp"
#include "toolshed/log/Logger.hpp"
#include "trunk/ports/TodoRepository.hpp"

namespace planning::application {

// Todo 부분 수정. 값이 있는 필드만 변경. tags 는 전체 교체.
class UpdateTodoUseCase {
public:
    UpdateTodoUseCase(ports::TodoRepository& todos, toolshed::log::Logger& logger);

    // 대상이 없으면 std::out_of_range.
    void execute(const UpdateTodoCommand& cmd);

private:
    ports::TodoRepository& todos_;
    toolshed::log::Logger& logger_;
};

}  // namespace planning::application
