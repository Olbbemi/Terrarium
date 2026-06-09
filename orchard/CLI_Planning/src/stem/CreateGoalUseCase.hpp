#pragma once

#include "seed/Goal.hpp"
#include "seed/IdGenerator.hpp"
#include "stem/commands/GoalCommands.hpp"
#include "stomata/GoalRepository.hpp"
#include "stomata/Logger.hpp"

namespace planning::application {

class CreateGoalUseCase {
public:
    CreateGoalUseCase(ports::GoalRepository& goals, domain::IdGenerator& idGen,
                      ports::Logger& logger);

    // 같은 이름이 이미 있으면 std::invalid_argument.
    // targetValue <= 0 또는 기간 역전 시에도 std::invalid_argument(Goal 불변식).
    domain::Goal::Id execute(const CreateGoalCommand& cmd);

private:
    ports::GoalRepository& goals_;
    domain::IdGenerator& idGen_;
    ports::Logger& logger_;
};

}  // namespace planning::application
