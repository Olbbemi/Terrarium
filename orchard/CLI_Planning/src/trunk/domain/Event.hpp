#pragma once

#include <optional>
#include <string>

#include "uuid.h"

#include "trunk/domain/RecurrenceRule.hpp"
#include "trunk/domain/TimeRange.hpp"

namespace planning::domain {

// 일정 이벤트 (Aggregate Root).
class Event {
public:
    using Id = uuids::uuid;

    // title 이 비어 있으면 std::invalid_argument 를 던진다.
    Event(Id id, std::string title, TimeRange range,
          std::optional<RecurrenceRule> recurrence = std::nullopt);

    Id id() const;
    const std::string& title() const;
    const TimeRange& timeRange() const;
    std::optional<RecurrenceRule> recurrenceRule() const;

    void reschedule(TimeRange newRange);
    void rename(std::string newTitle);  // 빈 제목이면 std::invalid_argument
    void setRecurrence(std::optional<RecurrenceRule> rule);

private:
    Id id_;
    std::string title_;
    TimeRange range_;
    std::optional<RecurrenceRule> recurrence_;
};

}  // namespace planning::domain
