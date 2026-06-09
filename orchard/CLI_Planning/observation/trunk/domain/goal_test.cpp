#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

#include "trunk/domain/Goal.hpp"

using planning::domain::Goal;

namespace {

std::chrono::sys_days day(int n) {
    return std::chrono::sys_days{std::chrono::days{n}};
}

Goal makeGoal(int target = 40) {
    return Goal(uuids::uuid{}, "운동", target, "회", day(20000), day(20100));
}

}  // namespace

// --- happy ---

TEST(Goal, increment_counter) {
    Goal g = makeGoal();
    EXPECT_EQ(g.currentValue(), 0);
    g.incrementCounter();
    g.incrementCounter();
    EXPECT_EQ(g.currentValue(), 2);
}

TEST(Goal, progress_ratio) {
    Goal g = makeGoal(4);
    g.incrementCounter();
    g.incrementCounter();
    EXPECT_DOUBLE_EQ(g.progressRatio(), 0.5);
}

TEST(Goal, rename) {
    Goal g = makeGoal();
    g.rename("러닝");
    EXPECT_EQ(g.name(), "러닝");
}

TEST(Goal, update_target) {
    Goal g = makeGoal();
    g.updateTarget(10);
    EXPECT_EQ(g.targetValue(), 10);
}

TEST(Goal, update_period) {
    Goal g = makeGoal();
    g.updatePeriod(day(21000), day(21100));
    EXPECT_EQ(g.periodStart(), day(21000));
    EXPECT_EQ(g.periodEnd(), day(21100));
}

// --- 실패 ---

TEST(Goal, throws_when_target_non_positive) {
    EXPECT_THROW(Goal(uuids::uuid{}, "운동", 0, "회", day(20000), day(20100)),
                 std::invalid_argument);
}

TEST(Goal, throws_when_period_invalid) {
    EXPECT_THROW(Goal(uuids::uuid{}, "운동", 40, "회", day(20100), day(20000)),
                 std::invalid_argument);
}

// --- edge ---

TEST(Goal, progress_ratio_over_100_not_capped) {
    Goal g = makeGoal(4);
    for (int i = 0; i < 5; ++i) {
        g.incrementCounter();
    }
    EXPECT_DOUBLE_EQ(g.progressRatio(), 1.25);  // 초과 달성, 상한 없음
}
