#pragma once

#include "trunk/usecase/commands/GoalCommands.hpp"
#include "trunk/ports/GoalRepository.hpp"
#include "toolshed/log/Logger.hpp"

namespace planning::application {

// goal log <name> — 이름으로 찾아 누적값 +1. 기간 종료 후에도 동작한다.
class LogGoalUseCase {
public:
    LogGoalUseCase(ports::GoalRepository& goals, toolshed::log::Logger& logger);

    // 이름에 해당하는 목표가 없으면 std::out_of_range.
    void execute(const LogGoalCommand& cmd);

private:
    ports::GoalRepository& goals_;
    toolshed::log::Logger& logger_;
};

}  // namespace planning::application
