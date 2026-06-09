#pragma once

#include "trunk/usecase/commands/TodoCommands.hpp"
#include "toolshed/log/Logger.hpp"
#include "trunk/ports/TodoRepository.hpp"

namespace planning::application {

class MarkTodoDoneUseCase {
public:
    MarkTodoDoneUseCase(ports::TodoRepository& todos, toolshed::log::Logger& logger);

    // 대상이 없으면 std::out_of_range. 이미 완료된 항목엔 멱등.
    void execute(const MarkTodoDoneCommand& cmd);

private:
    ports::TodoRepository& todos_;
    toolshed::log::Logger& logger_;
};

}  // namespace planning::application
