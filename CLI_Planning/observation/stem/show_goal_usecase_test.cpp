#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

#include "seed/Goal.hpp"
#include "stem/ShowGoalUseCase.hpp"
#include "stem/commands/GoalCommands.hpp"
#include "support/fakes.hpp"

using planning::application::ShowGoalQuery;
using planning::application::ShowGoalUseCase;
using planning::domain::Goal;

namespace {

std::chrono::sys_days day(int n) {
    return std::chrono::sys_days{std::chrono::days{n}};
}

uuids::uuid id(const char* s) { return uuids::uuid::from_string(s).value(); }

const char* kA = "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa";
const char* kB = "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb";

Goal goalWith(int target, int logs) {
    Goal g(id(kB), "운동", target, "회", day(20000), day(20100));
    for (int i = 0; i < logs; ++i) g.incrementCounter();
    return g;
}

}  // namespace

TEST(ShowGoalUseCase, returns_progress_ratio) {
    planning::test::FakeGoalRepository repo;
    repo.save(goalWith(4, 2));  // 2/4 = 0.5
    planning::test::FakeLogger logger;
    ShowGoalUseCase uc(repo, logger);

    auto r = uc.execute(ShowGoalQuery{id(kB)});
    EXPECT_EQ(r.currentValue, 2);
    EXPECT_EQ(r.targetValue, 4);
    EXPECT_DOUBLE_EQ(r.progressRatio, 0.5);
    EXPECT_EQ(r.name, "운동");
    EXPECT_EQ(r.unit, "회");
}

TEST(ShowGoalUseCase, completed_at_100_percent) {
    planning::test::FakeGoalRepository repo;
    repo.save(goalWith(4, 4));  // 4/4 = 1.0
    planning::test::FakeLogger logger;
    ShowGoalUseCase uc(repo, logger);

    auto r = uc.execute(ShowGoalQuery{id(kB)});
    EXPECT_DOUBLE_EQ(r.progressRatio, 1.0);
}

TEST(ShowGoalUseCase, throws_when_not_found) {
    planning::test::FakeGoalRepository repo;
    planning::test::FakeLogger logger;
    ShowGoalUseCase uc(repo, logger);

    EXPECT_THROW(uc.execute(ShowGoalQuery{id(kA)}), std::out_of_range);
}
