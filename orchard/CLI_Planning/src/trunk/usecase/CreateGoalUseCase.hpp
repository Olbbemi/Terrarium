#pragma once

#include "trunk/domain/Goal.hpp"
#include "trunk/domain/IdGenerator.hpp"
#include "trunk/usecase/commands/GoalCommands.hpp"
#include "trunk/ports/GoalRepository.hpp"
#include "toolshed/log/Logger.hpp"

namespace planning::application {

class CreateGoalUseCase {
public:
    CreateGoalUseCase(ports::GoalRepository& goals, domain::IdGenerator& idGen,
                      toolshed::log::Logger& logger);

    // 같은 이름이 이미 있으면 std::invalid_argument.
    // targetValue <= 0 또는 기간 역전 시에도 std::invalid_argument(Goal 불변식).
    domain::Goal::Id execute(const CreateGoalCommand& cmd);

private:
    ports::GoalRepository& goals_;
    domain::IdGenerator& idGen_;
    toolshed::log::Logger& logger_;
};

}  // namespace planning::application
