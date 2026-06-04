#pragma once

#include <vector>

#include "seed/Goal.hpp"
#include "stomata/GoalRepository.hpp"
#include "stomata/Logger.hpp"

namespace planning::application {

class ListGoalsUseCase {
public:
    ListGoalsUseCase(ports::GoalRepository& goals, ports::Logger& logger);

    std::vector<domain::Goal> execute();

private:
    ports::GoalRepository& goals_;
    ports::Logger& logger_;
};

}  // namespace planning::application
