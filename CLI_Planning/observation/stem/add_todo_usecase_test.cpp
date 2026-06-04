#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <stdexcept>

#include "seed/Priority.hpp"
#include "stem/AddTodoUseCase.hpp"
#include "stem/commands/TodoCommands.hpp"
#include "support/fakes.hpp"

using planning::application::AddTodoCommand;
using planning::application::AddTodoUseCase;
using planning::domain::Priority;

namespace {

std::chrono::sys_days day(int n) {
    return std::chrono::sys_days{std::chrono::days{n}};
}

}  // namespace

TEST(AddTodoUseCase, persists_with_tags) {
    planning::test::FakeTodoRepository repo;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeLogger logger;
    AddTodoUseCase uc(repo, idGen, logger);

    auto id = uc.execute(
        AddTodoCommand{"장보기", Priority::MEDIUM, {"home", "errand"}, day(20000)});
    auto saved = repo.findById(id);
    ASSERT_TRUE(saved.has_value());
    EXPECT_EQ(saved->tags().size(), 2u);
    ASSERT_TRUE(saved->dueDate().has_value());
    EXPECT_EQ(*saved->dueDate(), day(20000));
}

TEST(AddTodoUseCase, persists_without_due) {
    planning::test::FakeTodoRepository repo;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeLogger logger;
    AddTodoUseCase uc(repo, idGen, logger);

    auto id = uc.execute(
        AddTodoCommand{"운동", Priority::LOW, {}, std::nullopt});
    auto saved = repo.findById(id);
    ASSERT_TRUE(saved.has_value());
    EXPECT_FALSE(saved->dueDate().has_value());
}

TEST(AddTodoUseCase, throws_when_title_empty) {
    planning::test::FakeTodoRepository repo;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeLogger logger;
    AddTodoUseCase uc(repo, idGen, logger);

    EXPECT_THROW(uc.execute(AddTodoCommand{"", Priority::LOW, {}, std::nullopt}),
                 std::invalid_argument);
}

TEST(AddTodoUseCase, empty_tags_list) {
    planning::test::FakeTodoRepository repo;
    planning::test::FakeIdGenerator idGen;
    planning::test::FakeLogger logger;
    AddTodoUseCase uc(repo, idGen, logger);

    auto id = uc.execute(AddTodoCommand{"메모", Priority::LOW, {}, std::nullopt});
    auto saved = repo.findById(id);
    ASSERT_TRUE(saved.has_value());
    EXPECT_TRUE(saved->tags().empty());
}
