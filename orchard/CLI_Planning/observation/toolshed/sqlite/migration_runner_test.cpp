#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include <SQLiteCpp/SQLiteCpp.h>

#include "toolshed/sqlite/MigrationRunner.hpp"
#include "support/sqlite_test_db.hpp"

using toolshed::sqlite::MigrationRunner;
using planning::test::makeMigratedDb;
using planning::test::tableExists;

namespace {

namespace fs = std::filesystem;

fs::path migDir(const char* name) {
    fs::path d = fs::path(::testing::TempDir()) / (std::string("mig_") + name);
    fs::remove_all(d);
    fs::create_directories(d);
    return d;
}

void writeSql(const fs::path& p, const std::string& sql) {
    std::ofstream(p) << sql;
}

SQLite::Database memDb() {
    SQLite::Database db(":memory:",
                        SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db.exec("PRAGMA foreign_keys = ON");
    return db;
}

}  // namespace

TEST(MigrationRunner, initial_run_creates_all_tables) {
    auto db = makeMigratedDb();
    EXPECT_TRUE(tableExists(db, "events"));
    EXPECT_TRUE(tableExists(db, "todos"));
    EXPECT_TRUE(tableExists(db, "todo_tags"));
    EXPECT_TRUE(tableExists(db, "goals"));
}

TEST(MigrationRunner, skip_already_applied) {
    auto db = makeMigratedDb();
    // 한 번 더 실행해도 오류 없이 스킵.
    EXPECT_NO_THROW(MigrationRunner(db).run(TERRARIUM_MIGRATIONS_DIR));
    SQLite::Statement q(db, "SELECT COUNT(*) FROM schema_version");
    q.executeStep();
    EXPECT_EQ(q.getColumn(0).getInt(), 1);
}

TEST(MigrationRunner, apply_new_migration) {
    auto dir = migDir("apply");
    writeSql(dir / "001_a.sql", "CREATE TABLE a(x INTEGER);");
    writeSql(dir / "002_b.sql", "CREATE TABLE b(y INTEGER);");

    auto db = memDb();
    MigrationRunner(db).run(dir);
    EXPECT_TRUE(tableExists(db, "a"));
    EXPECT_TRUE(tableExists(db, "b"));

    SQLite::Statement q(db, "SELECT COUNT(*) FROM schema_version");
    q.executeStep();
    EXPECT_EQ(q.getColumn(0).getInt(), 2);
}

TEST(MigrationRunner, invalid_sql_rolls_back_transaction) {
    auto dir = migDir("invalid");
    writeSql(dir / "001_ok.sql", "CREATE TABLE ok(x INTEGER);");
    // bad 테이블 생성 후 존재하지 않는 테이블에 INSERT → 같은 트랜잭션에서 실패.
    writeSql(dir / "002_bad.sql",
             "CREATE TABLE bad(x INTEGER); INSERT INTO nope VALUES(1);");

    auto db = memDb();
    EXPECT_THROW(MigrationRunner(db).run(dir), SQLite::Exception);
    EXPECT_TRUE(tableExists(db, "ok"));    // 001 은 커밋됨
    EXPECT_FALSE(tableExists(db, "bad"));  // 002 는 롤백됨
}
