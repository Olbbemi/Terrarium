#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

#include "seed/Goal.hpp"
#include "stem/CreateGoalUseCase.hpp"
#include "stem/commands/GoalCommands.hpp"
#include "support/fakes.hpp"

using planning::application::CreateGoalCommand;
using planning::application::CreateGoalUseCase;
using planning::domain::Goal;

namespace {

std::chrono::sys_days day(int n) {
    return std::chrono::sys_days{std::chrono::days{n}};
}

uuids::uuid id(const char* s) { return uuids::uuid::from_string(s).value(); }

CreateGoalCommand cmd(const char* name) {
    return CreateGoalCommand{name, 40, "회", day(20000), day(20100)};
}

}  // namespace

TEST(CreateGoalUseCase, persists_with_required) {
    planning::test::FakeGoalRepository repo;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeLogger logger;
    CreateGoalUseCase uc(repo, idGen, logger);

    auto gid = uc.execute(cmd("운동"));
    auto saved = repo.findById(gid);
    ASSERT_TRUE(saved.has_value());
    EXPECT_EQ(saved->name(), "운동");
    EXPECT_EQ(saved->targetValue(), 40);
    EXPECT_EQ(saved->currentValue(), 0);
}

TEST(CreateGoalUseCase, throws_when_name_duplicate) {
    planning::test::FakeGoalRepository repo;
    repo.save(Goal(id("11111111-1111-1111-1111-111111111111"), "운동", 40, "회",
                   day(20000), day(20100)));
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeLogger logger;
    CreateGoalUseCase uc(repo, idGen, logger);

    EXPECT_THROW(uc.execute(cmd("운동")), std::invalid_argument);
}
