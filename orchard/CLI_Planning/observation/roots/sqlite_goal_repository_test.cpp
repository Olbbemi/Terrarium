#include <gtest/gtest.h>

#include <chrono>

#include <SQLiteCpp/SQLiteCpp.h>

#include "roots/SqliteGoalRepository.hpp"
#include "trunk/domain/Goal.hpp"
#include "support/sqlite_test_db.hpp"

using planning::store::SqliteGoalRepository;
using planning::domain::Goal;
using planning::test::makeMigratedDatabase;

namespace {

std::chrono::sys_days day(int n) {
    return std::chrono::sys_days{std::chrono::days{n}};
}

uuids::uuid id(const char* s) { return uuids::uuid::from_string(s).value(); }

Goal goal(const char* uuid, const char* name) {
    return Goal(uuids::uuid::from_string(uuid).value(), name, 40, "회",
                day(20000), day(20100));
}

const char* k1 = "11111111-1111-1111-1111-111111111111";
const char* k2 = "22222222-2222-2222-2222-222222222222";

}  // namespace

TEST(SqliteGoalRepository, findByName_returns_goal) {
    auto db = makeMigratedDatabase();
    SqliteGoalRepository repo(db);
    repo.save(goal(k1, "운동"));

    auto found = repo.findByName("운동");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name(), "운동");
    EXPECT_EQ(found->targetValue(), 40);
}

TEST(SqliteGoalRepository, save_roundtrip_preserves_counter) {
    auto db = makeMigratedDatabase();
    SqliteGoalRepository repo(db);
    Goal g = goal(k1, "운동");
    g.incrementCounter();
    g.incrementCounter();
    g.incrementCounter();
    repo.save(g);

    auto loaded = repo.findById(id(k1));
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->currentValue(), 3);  // 누적값 복원
}

TEST(SqliteGoalRepository, name_unique_constraint_throws_on_duplicate) {
    auto db = makeMigratedDatabase();
    SqliteGoalRepository repo(db);
    repo.save(goal(k1, "운동"));
    EXPECT_THROW(repo.save(goal(k2, "운동")), SQLite::Exception);
}

TEST(SqliteGoalRepository, rename_persists_name_change) {
    auto db = makeMigratedDatabase();
    SqliteGoalRepository repo(db);
    repo.save(goal(k1, "운동"));

    auto g = repo.findById(id(k1));
    ASSERT_TRUE(g.has_value());
    g->rename("러닝");
    repo.update(*g);

    EXPECT_TRUE(repo.findByName("러닝").has_value());
    EXPECT_FALSE(repo.findByName("운동").has_value());
}
