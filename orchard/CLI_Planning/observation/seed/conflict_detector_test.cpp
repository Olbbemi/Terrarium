#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <vector>

#include "seed/ConflictDetector.hpp"
#include "seed/Event.hpp"
#include "seed/TimeRange.hpp"

using planning::domain::ConflictDetector;
using planning::domain::Event;
using planning::domain::TimeRange;

namespace {

std::chrono::sys_seconds ts(long sec) {
    return std::chrono::sys_seconds{std::chrono::seconds{sec}};
}

Event ev(const char* title, long start, long end) {
    return Event(uuids::uuid{}, title, TimeRange(ts(start), ts(end)));
}

}  // namespace

// --- happy ---

TEST(ConflictDetector, returns_none_no_overlap) {
    ConflictDetector detector;
    TimeRange candidate(ts(100), ts(200));
    std::vector<Event> existing{ev("A", 300, 400), ev("B", 500, 600)};
    EXPECT_FALSE(detector.detect(candidate, existing).has_value());
}

TEST(ConflictDetector, returns_first_conflict_when_overlap) {
    ConflictDetector detector;
    TimeRange candidate(ts(100), ts(200));
    std::vector<Event> existing{ev("A", 300, 400), ev("B", 150, 250),
                                ev("C", 180, 220)};
    auto conflict = detector.detect(candidate, existing);
    ASSERT_TRUE(conflict.has_value());
    EXPECT_EQ(conflict->existingTitle, "B");  // 순서상 첫 충돌
}

// --- edge ---

TEST(ConflictDetector, empty_existing_returns_none) {
    ConflictDetector detector;
    TimeRange candidate(ts(100), ts(200));
    std::vector<Event> existing;
    EXPECT_FALSE(detector.detect(candidate, existing).has_value());
}
