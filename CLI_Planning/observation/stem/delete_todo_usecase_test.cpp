#include <gtest/gtest.h>

#include <stdexcept>

#include "seed/Priority.hpp"
#include "seed/Todo.hpp"
#include "stem/DeleteTodoUseCase.hpp"
#include "stem/commands/TodoCommands.hpp"
#include "support/fakes.hpp"

using planning::application::DeleteTodoCommand;
using planning::application::DeleteTodoUseCase;
using planning::domain::Priority;
using planning::domain::Todo;

namespace {

uuids::uuid id(const char* s) { return uuids::uuid::from_string(s).value(); }

const char* kA = "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa";
const char* kB = "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb";

}  // namespace

TEST(DeleteTodoUseCase, removes_todo_and_tags_cascade) {
    planning::test::FakeTodoRepository repo;
    repo.save(Todo(id(kB), "정리", Priority::LOW, {"home", "weekend"}));
    planning::test::FakeLogger logger;
    DeleteTodoUseCase uc(repo, logger);

    uc.execute(DeleteTodoCommand{id(kB)});
    EXPECT_FALSE(repo.findById(id(kB)).has_value());
    EXPECT_EQ(repo.size(), 0u);  // 태그는 Todo 에 내장 → 함께 제거
}

TEST(DeleteTodoUseCase, throws_when_not_found) {
    planning::test::FakeTodoRepository repo;
    planning::test::FakeLogger logger;
    DeleteTodoUseCase uc(repo, logger);

    EXPECT_THROW(uc.execute(DeleteTodoCommand{id(kA)}), std::out_of_range);
}
