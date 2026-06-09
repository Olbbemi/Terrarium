#pragma once

#include "trunk/usecase/commands/GoalCommands.hpp"
#include "trunk/ports/GoalRepository.hpp"
#include "toolshed/log/Logger.hpp"

namespace planning::application {

class DeleteGoalUseCase {
public:
    DeleteGoalUseCase(ports::GoalRepository& goals, toolshed::log::Logger& logger);

    // 대상이 없으면 std::out_of_range.
    void execute(const DeleteGoalCommand& cmd);

private:
    ports::GoalRepository& goals_;
    toolshed::log::Logger& logger_;
};

}  // namespace planning::application
