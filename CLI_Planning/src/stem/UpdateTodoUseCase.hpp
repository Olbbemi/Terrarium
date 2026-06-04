#pragma once

#include "stem/commands/TodoCommands.hpp"
#include "stomata/Logger.hpp"
#include "stomata/TodoRepository.hpp"

namespace planning::application {

// Todo 부분 수정. 값이 있는 필드만 변경. tags 는 전체 교체.
class UpdateTodoUseCase {
public:
    UpdateTodoUseCase(ports::TodoRepository& todos, ports::Logger& logger);

    // 대상이 없으면 std::out_of_range.
    void execute(const UpdateTodoCommand& cmd);

private:
    ports::TodoRepository& todos_;
    ports::Logger& logger_;
};

}  // namespace planning::application
