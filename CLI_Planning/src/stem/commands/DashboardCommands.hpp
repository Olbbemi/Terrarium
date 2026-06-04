#pragma once

#include <chrono>

namespace planning::application {

struct ShowDashboardQuery {
    std::chrono::sys_days today;
};

}  // namespace planning::application
