#include "trunk/usecase/ShowGoalUseCase.hpp"

#include <stdexcept>

#include "trunk/domain/Goal.hpp"

namespace planning::application {

ShowGoalUseCase::ShowGoalUseCase(ports::GoalRepository& goals,
                                 ports::Logger& logger)
    : goals_(goals), logger_(logger) {}

ShowGoalUseCase::Result ShowGoalUseCase::execute(const ShowGoalQuery& query) {
    auto found = goals_.findByName(query.name);
    if (!found) {
        throw std::out_of_range("ShowGoalUseCase: goal not found by name");
    }
    const domain::Goal& g = *found;
    logger_.audit("goal.show", "viewed");
    return Result{g.name(), g.currentValue(), g.targetValue(), g.unit(),
                  g.progressRatio()};
}

}  // namespace planning::application
