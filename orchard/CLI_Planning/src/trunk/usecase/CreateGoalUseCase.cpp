#include "trunk/usecase/CreateGoalUseCase.hpp"

#include <stdexcept>

namespace planning::application {

CreateGoalUseCase::CreateGoalUseCase(ports::GoalRepository& goals,
                                     domain::IdGenerator& idGen,
                                     ports::Logger& logger)
    : goals_(goals), idGen_(idGen), logger_(logger) {}

domain::Goal::Id CreateGoalUseCase::execute(const CreateGoalCommand& cmd) {
    if (goals_.findByName(cmd.name)) {
        throw std::invalid_argument("CreateGoalUseCase: goal name already exists");
    }
    const auto id = idGen_.next();
    domain::Goal goal(id, cmd.name, cmd.targetValue, cmd.unit, cmd.periodStart,
                      cmd.periodEnd);
    goals_.save(goal);
    logger_.audit("goal.create", "created");
    return id;
}

}  // namespace planning::application
