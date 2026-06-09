#pragma once

#include <vector>

#include "trunk/domain/Goal.hpp"
#include "trunk/ports/GoalRepository.hpp"
#include "trunk/ports/Logger.hpp"

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
