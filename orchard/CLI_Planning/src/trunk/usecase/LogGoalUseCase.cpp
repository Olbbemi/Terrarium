#include "trunk/usecase/LogGoalUseCase.hpp"

#include <stdexcept>

#include "trunk/domain/Goal.hpp"

namespace planning::application {

LogGoalUseCase::LogGoalUseCase(ports::GoalRepository& goals, ports::Logger& logger)
    : goals_(goals), logger_(logger) {}

void LogGoalUseCase::execute(const LogGoalCommand& cmd) {
    auto found = goals_.findByName(cmd.goalName);
    if (!found) {
        throw std::out_of_range("LogGoalUseCase: goal not found by name");
    }
    domain::Goal goal = *found;
    goal.incrementCounter();  // 기간 종료 여부와 무관하게 누적
    goals_.update(goal);
    logger_.audit("goal.log", "incremented");
}

}  // namespace planning::application
