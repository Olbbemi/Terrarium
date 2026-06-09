#pragma once

#include "trunk/usecase/commands/GoalCommands.hpp"
#include "trunk/ports/GoalRepository.hpp"
#include "toolshed/log/Logger.hpp"

namespace planning::application {

class UpdateGoalUseCase {
public:
    UpdateGoalUseCase(ports::GoalRepository& goals, toolshed::log::Logger& logger);

    // 대상이 없으면 std::out_of_range.
    // 다른 목표가 이미 쓰는 이름으로 변경 시 std::invalid_argument.
    void execute(const UpdateGoalCommand& cmd);

private:
    ports::GoalRepository& goals_;
    toolshed::log::Logger& logger_;
};

}  // namespace planning::application
