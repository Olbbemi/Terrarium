#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <utility>

#include "trunk/domain/Goal.hpp"

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

// goal show <name> — 이름으로 조회(저장소 findByName 사용, 전체 스캔 회피).
struct ShowGoalQuery {
    std::string name;
};

}  // namespace planning::application
