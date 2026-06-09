#pragma once

#include "trunk/usecase/commands/GoalCommands.hpp"
#include "trunk/ports/GoalRepository.hpp"
#include "trunk/ports/Logger.hpp"

namespace planning::application {

// goal log <name> — 이름으로 찾아 누적값 +1. 기간 종료 후에도 동작한다.
class LogGoalUseCase {
public:
    LogGoalUseCase(ports::GoalRepository& goals, ports::Logger& logger);

    // 이름에 해당하는 목표가 없으면 std::out_of_range.
    void execute(const LogGoalCommand& cmd);

private:
    ports::GoalRepository& goals_;
    ports::Logger& logger_;
};

}  // namespace planning::application
