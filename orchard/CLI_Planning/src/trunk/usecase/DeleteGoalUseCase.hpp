#pragma once

#include "trunk/usecase/commands/GoalCommands.hpp"
#include "trunk/ports/GoalRepository.hpp"
#include "trunk/ports/Logger.hpp"

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
