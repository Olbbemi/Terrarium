#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>

#include "seed/Event.hpp"
#include "seed/RecurrenceRule.hpp"
#include "seed/TimeRange.hpp"

using planning::domain::Event;
using planning::domain::RecurrenceFrequency;
using planning::domain::RecurrenceRule;
using planning::domain::TimeRange;

namespace {

std::chrono::sys_seconds ts(long sec) {
    return std::chrono::sys_seconds{std::chrono::seconds{sec}};
}

TimeRange sampleRange() {
    return TimeRange(ts(100), ts(200));
}

}  // namespace

// --- happy ---

TEST(Event, construct_with_required) {
    Event e(uuids::uuid{}, "회의", sampleRange());
    EXPECT_EQ(e.title(), "회의");
    EXPECT_EQ(e.timeRange().start(), ts(100));
    EXPECT_FALSE(e.recurrenceRule().has_value());
}

TEST(Event, reschedule_updates_range) {
    Event e(uuids::uuid{}, "회의", sampleRange());
    e.reschedule(TimeRange(ts(500), ts(600)));
    EXPECT_EQ(e.timeRange().start(), ts(500));
    ASSERT_TRUE(e.timeRange().end().has_value());
    EXPECT_EQ(*e.timeRange().end(), ts(600));
}

TEST(Event, set_recurrence_persists) {
    Event e(uuids::uuid{}, "주간회의", sampleRange());
    RecurrenceRule rule{RecurrenceFrequency::Weekly, ts(10000)};
    e.setRecurrence(rule);
    ASSERT_TRUE(e.recurrenceRule().has_value());
    EXPECT_EQ(e.recurrenceRule()->frequency, RecurrenceFrequency::Weekly);
    ASSERT_TRUE(e.recurrenceRule()->until.has_value());
    EXPECT_EQ(*e.recurrenceRule()->until, ts(10000));
}

// --- 실패 ---

TEST(Event, throws_when_title_empty) {
    EXPECT_THROW(Event(uuids::uuid{}, "", sampleRange()), std::invalid_argument);
}

// --- edge ---

TEST(Event, recurrence_until_null_infinite) {
    RecurrenceRule infinite{RecurrenceFrequency::Daily, std::nullopt};
    Event e(uuids::uuid{}, "매일운동", sampleRange(), infinite);
    ASSERT_TRUE(e.recurrenceRule().has_value());
    EXPECT_EQ(e.recurrenceRule()->frequency, RecurrenceFrequency::Daily);
    EXPECT_FALSE(e.recurrenceRule()->until.has_value());  // nullopt = 무한 반복
}
