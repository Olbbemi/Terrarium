#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

#include "seed/Goal.hpp"
#include "stem/DeleteGoalUseCase.hpp"
#include "stem/commands/GoalCommands.hpp"
#include "support/fakes.hpp"

using planning::application::DeleteGoalCommand;
using planning::application::DeleteGoalUseCase;
using planning::domain::Goal;

namespace {

std::chrono::sys_days day(int n) {
    return std::chrono::sys_days{std::chrono::days{n}};
}

uuids::uuid id(const char* s) { return uuids::uuid::from_string(s).value(); }

const char* kA = "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa";
const char* kB = "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb";

}  // namespace

TEST(DeleteGoalUseCase, removes_goal) {
    planning::test::FakeGoalRepository repo;
    repo.save(Goal(id(kB), "운동", 40, "회", day(20000), day(20100)));
    planning::test::FakeLogger logger;
    DeleteGoalUseCase uc(repo, logger);

    uc.execute(DeleteGoalCommand{id(kB)});
    EXPECT_FALSE(repo.findById(id(kB)).has_value());
    EXPECT_EQ(repo.size(), 0u);
}

TEST(DeleteGoalUseCase, throws_when_not_found) {
    planning::test::FakeGoalRepository repo;
    planning::test::FakeLogger logger;
    DeleteGoalUseCase uc(repo, logger);

    EXPECT_THROW(uc.execute(DeleteGoalCommand{id(kA)}), std::out_of_range);
}
