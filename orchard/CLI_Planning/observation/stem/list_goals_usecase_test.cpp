#include <gtest/gtest.h>

#include <chrono>

#include "seed/Goal.hpp"
#include "stem/ListGoalsUseCase.hpp"
#include "support/fakes.hpp"

using planning::application::ListGoalsUseCase;
using planning::domain::Goal;

namespace {

std::chrono::sys_days day(int n) {
    return std::chrono::sys_days{std::chrono::days{n}};
}

uuids::uuid id(const char* s) { return uuids::uuid::from_string(s).value(); }

const char* kA = "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa";
const char* kB = "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb";

}  // namespace

TEST(ListGoalsUseCase, empty_returns_empty) {
    planning::test::FakeGoalRepository repo;
    planning::test::FakeLogger logger;
    ListGoalsUseCase uc(repo, logger);
    EXPECT_TRUE(uc.execute().empty());
}

TEST(ListGoalsUseCase, returns_all_goals) {
    planning::test::FakeGoalRepository repo;
    repo.save(Goal(id(kA), "운동", 40, "회", day(20000), day(20100)));
    repo.save(Goal(id(kB), "독서", 12, "권", day(20000), day(20100)));
    planning::test::FakeLogger logger;
    ListGoalsUseCase uc(repo, logger);

    EXPECT_EQ(uc.execute().size(), 2u);
}
