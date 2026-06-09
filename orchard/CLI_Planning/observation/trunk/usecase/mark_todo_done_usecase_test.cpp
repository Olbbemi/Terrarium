#include <gtest/gtest.h>

#include <stdexcept>

#include "trunk/domain/Priority.hpp"
#include "trunk/domain/Todo.hpp"
#include "trunk/usecase/MarkTodoDoneUseCase.hpp"
#include "trunk/usecase/commands/TodoCommands.hpp"
#include "support/fakes.hpp"

using planning::application::MarkTodoDoneCommand;
using planning::application::MarkTodoDoneUseCase;
using planning::domain::Priority;
using planning::domain::Todo;

namespace {

uuids::uuid id(const char* s) { return uuids::uuid::from_string(s).value(); }

const char* kA = "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa";
const char* kB = "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb";

}  // namespace

TEST(MarkTodoDoneUseCase, updates_status) {
    planning::test::FakeTodoRepository repo;
    repo.save(Todo(id(kB), "장보기", Priority::LOW, {}));
    planning::test::FakeLogger logger;
    MarkTodoDoneUseCase uc(repo, logger);

    uc.execute(MarkTodoDoneCommand{id(kB)});
    EXPECT_TRUE(repo.findById(id(kB))->isDone());
}

TEST(MarkTodoDoneUseCase, audit_logged) {
    planning::test::FakeTodoRepository repo;
    repo.save(Todo(id(kB), "장보기", Priority::LOW, {}));
    planning::test::FakeLogger logger;
    MarkTodoDoneUseCase uc(repo, logger);

    uc.execute(MarkTodoDoneCommand{id(kB)});
    EXPECT_FALSE(logger.auditLog.empty());
}

TEST(MarkTodoDoneUseCase, throws_when_not_found) {
    planning::test::FakeTodoRepository repo;
    planning::test::FakeLogger logger;
    MarkTodoDoneUseCase uc(repo, logger);

    EXPECT_THROW(uc.execute(MarkTodoDoneCommand{id(kA)}), std::out_of_range);
}

TEST(MarkTodoDoneUseCase, idempotent_for_already_done) {
    planning::test::FakeTodoRepository repo;
    repo.save(Todo(id(kB), "장보기", Priority::LOW, {}));
    planning::test::FakeLogger logger;
    MarkTodoDoneUseCase uc(repo, logger);

    uc.execute(MarkTodoDoneCommand{id(kB)});
    EXPECT_NO_THROW(uc.execute(MarkTodoDoneCommand{id(kB)}));
    EXPECT_TRUE(repo.findById(id(kB))->isDone());
}
