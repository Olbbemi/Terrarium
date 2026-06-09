#pragma once

#include "trunk/usecase/commands/GoalCommands.hpp"
#include "trunk/ports/GoalRepository.hpp"
#include "trunk/ports/Logger.hpp"

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
