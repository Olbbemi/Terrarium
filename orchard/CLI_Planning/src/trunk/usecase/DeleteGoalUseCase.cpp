#include "trunk/usecase/DeleteGoalUseCase.hpp"

#include <stdexcept>

namespace planning::application {

DeleteGoalUseCase::DeleteGoalUseCase(ports::GoalRepository& goals,
                                     toolshed::log::Logger& logger)
    : goals_(goals), logger_(logger) {}

void DeleteGoalUseCase::execute(const DeleteGoalCommand& cmd) {
    if (!goals_.findById(cmd.id)) {
        throw std::out_of_range("DeleteGoalUseCase: goal not found");
    }
    goals_.remove(cmd.id);
    logger_.audit("goal.delete", "deleted");
}

}  // namespace planning::application
