#pragma once

#include "stem/commands/GoalCommands.hpp"
#include "stomata/GoalRepository.hpp"
#include "stomata/Logger.hpp"

namespace planning::application {

class DeleteGoalUseCase {
public:
    DeleteGoalUseCase(ports::GoalRepository& goals, ports::Logger& logger);

    // 대상이 없으면 std::out_of_range.
    void execute(const DeleteGoalCommand& cmd);

private:
    ports::GoalRepository& goals_;
    ports::Logger& logger_;
};

}  // namespace planning::application
