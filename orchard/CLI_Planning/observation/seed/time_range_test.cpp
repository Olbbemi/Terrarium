#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <stdexcept>

#include "seed/TimeRange.hpp"

using planning::domain::TimeRange;

namespace {

// 초 단위 정수로 sys_seconds 시점을 만드는 테스트 헬퍼.
std::chrono::sys_seconds ts(long sec) {
    return std::chrono::sys_seconds{std::chrono::seconds{sec}};
}

}  // namespace

// --- happy ---

TEST(TimeRange, construct_valid) {
    TimeRange r(ts(100), ts(200));
    EXPECT_EQ(r.start(), ts(100));
    ASSERT_TRUE(r.end().has_value());
    EXPECT_EQ(*r.end(), ts(200));
    EXPECT_FALSE(r.isAllDay());
}

TEST(TimeRange, overlaps_true) {
    TimeRange a(ts(100), ts(200));
    TimeRange b(ts(150), ts(250));
    EXPECT_TRUE(a.overlaps(b));
    EXPECT_TRUE(b.overlaps(a));
}

TEST(TimeRange, overlaps_false) {
    TimeRange a(ts(100), ts(200));
    TimeRange b(ts(300), ts(400));
    EXPECT_FALSE(a.overlaps(b));
    EXPECT_FALSE(b.overlaps(a));
}

TEST(TimeRange, all_day_no_end) {
    TimeRange r(ts(0), std::nullopt, true);
    EXPECT_TRUE(r.isAllDay());
    EXPECT_FALSE(r.end().has_value());
}

// --- 실패 ---

TEST(TimeRange, throws_when_start_after_end) {
    EXPECT_THROW(TimeRange(ts(200), ts(100)), std::invalid_argument);
}

// --- edge ---

TEST(TimeRange, back_to_back_no_overlap) {
    TimeRange a(ts(100), ts(200));
    TimeRange b(ts(200), ts(300));
    EXPECT_FALSE(a.overlaps(b));
    EXPECT_FALSE(b.overlaps(a));
}

TEST(TimeRange, overlaps_self) {
    TimeRange r(ts(100), ts(200));
    EXPECT_TRUE(r.overlaps(r));
}
