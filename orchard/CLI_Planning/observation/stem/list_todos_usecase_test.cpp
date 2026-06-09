#include <gtest/gtest.h>

#include <chrono>

#include "seed/Priority.hpp"
#include "seed/Todo.hpp"
#include "stem/ListTodosUseCase.hpp"
#include "stem/commands/TodoCommands.hpp"
#include "support/fakes.hpp"

using planning::application::ListTodosQuery;
using planning::application::ListTodosUseCase;
using planning::domain::Priority;
using planning::domain::Todo;

namespace {

std::chrono::sys_days day(int n) {
    return std::chrono::sys_days{std::chrono::days{n}};
}

Todo todo(const char* uuid, const char* title, Priority p,
          std::vector<std::string> tags,
          std::optional<std::chrono::sys_days> due = std::nullopt) {
    return Todo(uuids::uuid::from_string(uuid).value(), title, p,
                std::move(tags), due);
}

const char* k1 = "11111111-1111-1111-1111-111111111111";
const char* k2 = "22222222-2222-2222-2222-222222222222";
const char* k3 = "33333333-3333-3333-3333-333333333333";

}  // namespace

TEST(ListTodosUseCase, empty_returns_empty) {
    planning::test::FakeTodoRepository repo;
    planning::test::FakeLogger logger;
    ListTodosUseCase uc(repo, logger);
    EXPECT_TRUE(uc.execute(ListTodosQuery{}).empty());
}

TEST(ListTodosUseCase, filter_today) {
    planning::test::FakeTodoRepository repo;
    repo.save(todo(k1, "오늘", Priority::LOW, {}, day(20000)));
    repo.save(todo(k2, "나중", Priority::LOW, {}, day(20005)));
    planning::test::FakeLogger logger;
    ListTodosUseCase uc(repo, logger);

    ListTodosQuery q;
    q.dueOn = day(20000);
    auto result = uc.execute(q);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].title(), "오늘");
}

TEST(ListTodosUseCase, filter_overdue) {
    planning::test::FakeTodoRepository repo;
    repo.save(todo(k1, "지남미완료", Priority::LOW, {}, day(19990)));
    repo.save(todo(k2, "미래", Priority::LOW, {}, day(20010)));
    Todo done = todo(k3, "지남완료", Priority::LOW, {}, day(19990));
    done.markDone();
    repo.save(done);
    planning::test::FakeLogger logger;
    ListTodosUseCase uc(repo, logger);

    ListTodosQuery q;
    q.overdueAsOf = day(20000);
    auto result = uc.execute(q);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].title(), "지남미완료");
}

TEST(ListTodosUseCase, filter_tag) {
    planning::test::FakeTodoRepository repo;
    repo.save(todo(k1, "A", Priority::LOW, {"home"}));
    repo.save(todo(k2, "B", Priority::LOW, {"work"}));
    planning::test::FakeLogger logger;
    ListTodosUseCase uc(repo, logger);

    ListTodosQuery q;
    q.tag = "home";
    auto result = uc.execute(q);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].title(), "A");
}

TEST(ListTodosUseCase, filter_priority) {
    planning::test::FakeTodoRepository repo;
    repo.save(todo(k1, "급함", Priority::HIGH, {}));
    repo.save(todo(k2, "보통", Priority::MEDIUM, {}));
    planning::test::FakeLogger logger;
    ListTodosUseCase uc(repo, logger);

    ListTodosQuery q;
    q.priority = Priority::HIGH;
    auto result = uc.execute(q);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].title(), "급함");
}

TEST(ListTodosUseCase, combined_filters) {
    planning::test::FakeTodoRepository repo;
    repo.save(todo(k1, "타깃", Priority::HIGH, {"home"}));
    repo.save(todo(k2, "태그불일치", Priority::HIGH, {"work"}));
    repo.save(todo(k3, "우선순위불일치", Priority::LOW, {"home"}));
    planning::test::FakeLogger logger;
    ListTodosUseCase uc(repo, logger);

    ListTodosQuery q;
    q.tag = "home";
    q.priority = Priority::HIGH;
    auto result = uc.execute(q);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].title(), "타깃");
}
