#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <utility>

#include "seed/Goal.hpp"

namespace planning::application {

struct CreateGoalCommand {
    std::string name;
    int targetValue;
    std::string unit;
    std::chrono::sys_days periodStart;
    std::chrono::sys_days periodEnd;
};

struct UpdateGoalCommand {
    domain::Goal::Id id;
    std::optional<std::string> name;
    std::optional<int> targetValue;
    std::optional<std::pair<std::chrono::sys_days, std::chrono::sys_days>> period;
};

struct DeleteGoalCommand {
    domain::Goal::Id id;
};

// goal log <name> — 이름으로 누적.
struct LogGoalCommand {
    std::string goalName;
};

struct ShowGoalQuery {
    domain::Goal::Id id;
};

}  // namespace planning::application
