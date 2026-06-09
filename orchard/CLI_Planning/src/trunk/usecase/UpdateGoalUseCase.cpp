#include "trunk/usecase/UpdateGoalUseCase.hpp"

#include <stdexcept>

#include "trunk/domain/Goal.hpp"

namespace planning::application {

UpdateGoalUseCase::UpdateGoalUseCase(ports::GoalRepository& goals,
                                     ports::Logger& logger)
    : goals_(goals), logger_(logger) {}

void UpdateGoalUseCase::execute(const UpdateGoalCommand& cmd) {
    auto found = goals_.findById(cmd.id);
    if (!found) {
        throw std::out_of_range("UpdateGoalUseCase: goal not found");
    }
    domain::Goal goal = *found;

    if (cmd.name) {
        auto existing = goals_.findByName(*cmd.name);
        if (existing && existing->id() != cmd.id) {
            throw std::invalid_argument("UpdateGoalUseCase: name already used");
        }
        goal.rename(*cmd.name);
    }
    if (cmd.targetValue) goal.updateTarget(*cmd.targetValue);
    if (cmd.period) goal.updatePeriod(cmd.period->first, cmd.period->second);

    goals_.update(goal);
    logger_.audit("goal.update", "updated");
}

}  // namespace planning::application
