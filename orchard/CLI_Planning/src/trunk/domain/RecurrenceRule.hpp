#pragma once

#include <chrono>
#include <optional>

namespace planning::domain {

enum class RecurrenceFrequency { Daily, Weekly, Monthly, Yearly };

// 이벤트 반복 규칙(값 객체). until 이 없으면(nullopt) 무한 반복.
struct RecurrenceRule {
    RecurrenceFrequency frequency;
    std::optional<std::chrono::sys_seconds> until;
};

}  // namespace planning::domain
