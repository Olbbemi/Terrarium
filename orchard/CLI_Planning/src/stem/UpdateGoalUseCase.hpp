#pragma once

#include "stem/commands/GoalCommands.hpp"
#include "stomata/GoalRepository.hpp"
#include "stomata/Logger.hpp"

namespace planning::application {

class UpdateGoalUseCase {
public:
    UpdateGoalUseCase(ports::GoalRepository& goals, ports::Logger& logger);

    // 대상이 없으면 std::out_of_range.
    // 다른 목표가 이미 쓰는 이름으로 변경 시 std::invalid_argument.
    void execute(const UpdateGoalCommand& cmd);

private:
    ports::GoalRepository& goals_;
    ports::Logger& logger_;
};

}  // namespace planning::application
