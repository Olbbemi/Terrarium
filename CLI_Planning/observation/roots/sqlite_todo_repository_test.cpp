#include <gtest/gtest.h>

#include <chrono>

#include <SQLiteCpp/SQLiteCpp.h>

#include "roots/SqliteTodoRepository.hpp"
#include "seed/Priority.hpp"
#include "seed/Todo.hpp"
#include "support/sqlite_test_db.hpp"

using planning::adapter_sqlite::SqliteTodoRepository;
using planning::domain::Priority;
using planning::domain::Todo;
using planning::test::makeMigratedDb;

namespace {

std::chrono::sys_days day(int n) {
    return std::chrono::sys_days{std::chrono::days{n}};
}

uuids::uuid id(const char* s) { return uuids::uuid::from_string(s).value(); }

int countRows(SQLite::Database& db, const std::string& sql, const std::string& bind) {
    SQLite::Statement q(db, sql);
    q.bind(1, bind);
    q.executeStep();
    return q.getColumn(0).getInt();
}

const char* k1 = "11111111-1111-1111-1111-111111111111";
const char* k2 = "22222222-2222-2222-2222-222222222222";

}  // namespace

TEST(SqliteTodoRepository, save_with_tags_persists_join_rows) {
    auto db = makeMigratedDb();
    SqliteTodoRepository repo(db);
    repo.save(Todo(id(k1), "장보기", Priority::MEDIUM, {"home", "errand"}, day(20000)));

    EXPECT_EQ(countRows(db, "SELECT COUNT(*) FROM todo_tags WHERE todo_id = ?",
                        uuids::to_string(id(k1))),
              2);
    auto loaded = repo.findById(id(k1));
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->tags().size(), 2u);
    ASSERT_TRUE(loaded->dueDate().has_value());
    EXPECT_EQ(*loaded->dueDate(), day(20000));
}

TEST(SqliteTodoRepository, remove_cascades_tags) {
    auto db = makeMigratedDb();
    SqliteTodoRepository repo(db);
    repo.save(Todo(id(k1), "정리", Priority::LOW, {"home", "weekend"}));
    repo.remove(id(k1));

    EXPECT_FALSE(repo.findById(id(k1)).has_value());
    EXPECT_EQ(countRows(db, "SELECT COUNT(*) FROM todo_tags WHERE todo_id = ?",
                        uuids::to_string(id(k1))),
              0);
}

TEST(SqliteTodoRepository, findByTag_returns_matching) {
    auto db = makeMigratedDb();
    SqliteTodoRepository repo(db);
    repo.save(Todo(id(k1), "A", Priority::LOW, {"home"}));
    repo.save(Todo(id(k2), "B", Priority::LOW, {"work"}));

    auto result = repo.findByTag("home");
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].title(), "A");
}

TEST(SqliteTodoRepository, overdue_excludes_done) {
    auto db = makeMigratedDb();
    SqliteTodoRepository repo(db);
    repo.save(Todo(id(k1), "지남미완료", Priority::LOW, {}, day(19990)));
    Todo done(id(k2), "지남완료", Priority::LOW, {}, day(19990));
    done.markDone();
    repo.save(done);

    auto result = repo.findOverdue(day(20000));
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].title(), "지남미완료");
}

TEST(SqliteTodoRepository, priority_invalid_throws_check_constraint) {
    auto db = makeMigratedDb();
    // 도메인을 우회한 raw INSERT 로 CHECK(priority IN (0,1,2)) 위반.
    EXPECT_THROW(
        db.exec("INSERT INTO todos (id, title, done, priority) "
                "VALUES ('x', 't', 0, 9)"),
        SQLite::Exception);
}
