#pragma once

#include <vector>

#include "trunk/domain/Goal.hpp"
#include "trunk/ports/GoalRepository.hpp"
#include "toolshed/log/Logger.hpp"

namespace planning::application {

class ListGoalsUseCase {
public:
    ListGoalsUseCase(ports::GoalRepository& goals, toolshed::log::Logger& logger);

    std::vector<domain::Goal> execute();

private:
    ports::GoalRepository& goals_;
    toolshed::log::Logger& logger_;
};

}  // namespace planning::application
