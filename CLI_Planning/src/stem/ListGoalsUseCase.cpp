#include "stem/ListGoalsUseCase.hpp"

namespace planning::application {

ListGoalsUseCase::ListGoalsUseCase(ports::GoalRepository& goals,
                                   ports::Logger& logger)
    : goals_(goals), logger_(logger) {}

std::vector<domain::Goal> ListGoalsUseCase::execute() {
    auto all = goals_.findAll();
    logger_.audit("goal.list", "listed");
    return all;
}

}  // namespace planning::application
