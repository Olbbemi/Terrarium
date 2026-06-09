#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

#include "seed/Goal.hpp"
#include "stem/UpdateGoalUseCase.hpp"
#include "stem/commands/GoalCommands.hpp"
#include "support/fakes.hpp"

using planning::application::UpdateGoalCommand;
using planning::application::UpdateGoalUseCase;
using planning::domain::Goal;

namespace {

std::chrono::sys_days day(int n) {
    return std::chrono::sys_days{std::chrono::days{n}};
}

uuids::uuid id(const char* s) { return uuids::uuid::from_string(s).value(); }

Goal goal(const char* uuid, const char* name) {
    return Goal(uuids::uuid::from_string(uuid).value(), name, 40, "회",
                day(20000), day(20100));
}

const char* kA = "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa";
const char* kB = "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb";

}  // namespace

TEST(UpdateGoalUseCase, persists_changes) {
    planning::test::FakeGoalRepository repo;
    repo.save(goal(kB, "운동"));
    planning::test::FakeLogger logger;
    UpdateGoalUseCase uc(repo, logger);

    UpdateGoalCommand cmd;
    cmd.id = id(kB);
    cmd.targetValue = 50;

    uc.execute(cmd);
    EXPECT_EQ(repo.findById(id(kB))->targetValue(), 50);
}

TEST(UpdateGoalUseCase, throws_when_not_found) {
    planning::test::FakeGoalRepository repo;
    planning::test::FakeLogger logger;
    UpdateGoalUseCase uc(repo, logger);

    UpdateGoalCommand cmd;
    cmd.id = id(kA);
    cmd.targetValue = 10;
    EXPECT_THROW(uc.execute(cmd), std::out_of_range);
}

TEST(UpdateGoalUseCase, rename_to_existing_throws) {
    planning::test::FakeGoalRepository repo;
    repo.save(goal(kA, "운동"));
    repo.save(goal(kB, "독서"));
    planning::test::FakeLogger logger;
    UpdateGoalUseCase uc(repo, logger);

    UpdateGoalCommand cmd;
    cmd.id = id(kB);
    cmd.name = "운동";  // 이미 kA 가 사용 중
    EXPECT_THROW(uc.execute(cmd), std::invalid_argument);
}
