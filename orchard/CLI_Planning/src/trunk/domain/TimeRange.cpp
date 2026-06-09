#include "trunk/domain/TimeRange.hpp"

#include <stdexcept>

namespace planning::domain {

TimeRange::TimeRange(std::chrono::sys_seconds start,
                     std::optional<std::chrono::sys_seconds> end,
                     bool allDay)
    : start_(start), end_(end), allDay_(allDay) {
    if (end_ && *end_ < start_) {
        throw std::invalid_argument("TimeRange: start must be <= end");
    }
}

std::chrono::sys_seconds TimeRange::start() const { return start_; }

std::optional<std::chrono::sys_seconds> TimeRange::end() const { return end_; }

bool TimeRange::isAllDay() const { return allDay_; }

std::chrono::sys_seconds TimeRange::effectiveEnd() const {
    return end_.value_or(start_);
}

bool TimeRange::overlaps(const TimeRange& other) const {
    return start_ < other.effectiveEnd() && other.start_ < effectiveEnd();
}

}  // namespace planning::domain
