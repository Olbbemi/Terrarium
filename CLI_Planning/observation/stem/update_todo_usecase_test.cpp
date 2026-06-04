#include <gtest/gtest.h>

#include <stdexcept>

#include "seed/Priority.hpp"
#include "seed/Todo.hpp"
#include "stem/UpdateTodoUseCase.hpp"
#include "stem/commands/TodoCommands.hpp"
#include "support/fakes.hpp"

using planning::application::UpdateTodoCommand;
using planning::application::UpdateTodoUseCase;
using planning::domain::Priority;
using planning::domain::Todo;

namespace {

uuids::uuid id(const char* s) { return uuids::uuid::from_string(s).value(); }

const char* kA = "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa";
const char* kB = "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb";

}  // namespace

TEST(UpdateTodoUseCase, persists_partial_changes) {
    planning::test::FakeTodoRepository repo;
    repo.save(Todo(id(kB), "원제목", Priority::LOW, {"a"}));
    planning::test::FakeLogger logger;
    UpdateTodoUseCase uc(repo, logger);

    UpdateTodoCommand cmd;
    cmd.id = id(kB);
    cmd.priority = Priority::HIGH;  // 우선순위만 변경

    uc.execute(cmd);
    auto updated = repo.findById(id(kB));
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->priority(), Priority::HIGH);
    EXPECT_EQ(updated->title(), "원제목");        // 유지
    ASSERT_EQ(updated->tags().size(), 1u);        // 유지
}

TEST(UpdateTodoUseCase, throws_when_not_found) {
    planning::test::FakeTodoRepository repo;
    planning::test::FakeLogger logger;
    UpdateTodoUseCase uc(repo, logger);

    UpdateTodoCommand cmd;
    cmd.id = id(kA);
    cmd.title = "x";
    EXPECT_THROW(uc.execute(cmd), std::out_of_range);
}

TEST(UpdateTodoUseCase, rename_to_same_value_no_op) {
    planning::test::FakeTodoRepository repo;
    repo.save(Todo(id(kB), "동일", Priority::LOW, {}));
    planning::test::FakeLogger logger;
    UpdateTodoUseCase uc(repo, logger);

    UpdateTodoCommand cmd;
    cmd.id = id(kB);
    cmd.title = "동일";  // 같은 값으로 rename

    EXPECT_NO_THROW(uc.execute(cmd));
    EXPECT_EQ(repo.findById(id(kB))->title(), "동일");
}
