#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

#include "trunk/domain/Goal.hpp"
#include "trunk/usecase/LogGoalUseCase.hpp"
#include "trunk/usecase/commands/GoalCommands.hpp"
#include "support/fakes.hpp"

using planning::application::LogGoalCommand;
using planning::application::LogGoalUseCase;
using planning::domain::Goal;

namespace {

std::chrono::sys_days day(int n) {
    return std::chrono::sys_days{std::chrono::days{n}};
}

uuids::uuid id(const char* s) { return uuids::uuid::from_string(s).value(); }

const char* kB = "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb";

}  // namespace

TEST(LogGoalUseCase, increments_by_one_and_persists) {
    planning::test::FakeGoalRepository repo;
    repo.save(Goal(id(kB), "운동", 40, "회", day(20000), day(20100)));
    planning::test::FakeLogger logger;
    LogGoalUseCase uc(repo, logger);

    uc.execute(LogGoalCommand{"운동"});
    EXPECT_EQ(repo.findById(id(kB))->currentValue(), 1);
    uc.execute(LogGoalCommand{"운동"});
    EXPECT_EQ(repo.findById(id(kB))->currentValue(), 2);
}

TEST(LogGoalUseCase, throws_when_not_found_by_name) {
    planning::test::FakeGoalRepository repo;
    planning::test::FakeLogger logger;
    LogGoalUseCase uc(repo, logger);

    EXPECT_THROW(uc.execute(LogGoalCommand{"없는목표"}), std::out_of_range);
}

TEST(LogGoalUseCase, after_period_end_still_works) {
    planning::test::FakeGoalRepository repo;
    // 기간이 이미 끝난(과거) 목표
    repo.save(Goal(id(kB), "운동", 40, "회", day(18000), day(19000)));
    planning::test::FakeLogger logger;
    LogGoalUseCase uc(repo, logger);

    uc.execute(LogGoalCommand{"운동"});
    EXPECT_EQ(repo.findById(id(kB))->currentValue(), 1);
}
