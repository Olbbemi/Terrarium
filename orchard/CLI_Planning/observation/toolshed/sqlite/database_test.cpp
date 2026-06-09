#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include <SQLiteCpp/SQLiteCpp.h>

#include "toolshed/sqlite/Database.hpp"

using toolshed::sqlite::Database;

namespace {

namespace fs = std::filesystem;

// 매 테스트 고유 임시 DB 경로(이전 잔여 제거).
fs::path dbPath(const char* name) {
    fs::path p = fs::path(::testing::TempDir()) / (std::string("tsq_") + name);
    fs::remove(p);
    fs::remove(fs::path(p) += "-wal");
    fs::remove(fs::path(p) += "-shm");
    return p;
}

std::string pragmaText(SQLite::Database& db, const char* pragma) {
    SQLite::Statement q(db, std::string("PRAGMA ") + pragma);
    q.executeStep();
    return q.getColumn(0).getString();
}

int pragmaInt(SQLite::Database& db, const char* pragma) {
    SQLite::Statement q(db, std::string("PRAGMA ") + pragma);
    q.executeStep();
    return q.getColumn(0).getInt();
}

}  // namespace

TEST(Database, open_creates_and_opens_db_file) {
    const auto p = dbPath("create");
    auto db = Database::open(p);
    EXPECT_TRUE(fs::exists(p));
}

TEST(Database, open_applies_wal_foreign_keys_busy_timeout) {
    const auto p = dbPath("pragmas");
    auto db = Database::open(p);
    EXPECT_EQ(pragmaText(db.handle(), "journal_mode"), "wal");
    EXPECT_EQ(pragmaInt(db.handle(), "foreign_keys"), 1);
    EXPECT_GT(pragmaInt(db.handle(), "busy_timeout"), 0);
}

TEST(Database, handle_exposes_sqlitecpp_for_direct_sql) {
    const auto p = dbPath("handle");
    auto db = Database::open(p);
    db.handle().exec("CREATE TABLE t(x INTEGER)");
    db.handle().exec("INSERT INTO t(x) VALUES(42)");
    SQLite::Statement q(db.handle(), "SELECT x FROM t");
    ASSERT_TRUE(q.executeStep());
    EXPECT_EQ(q.getColumn(0).getInt(), 42);
}

TEST(Database, open_throws_on_unwritable_path) {
    // 존재하지 않는 디렉토리 하위 → SQLite 가 열기 실패로 예외.
    const fs::path p = fs::path("/nonexistent_dir_xyzzy") / "no.db";
    EXPECT_THROW(Database::open(p), SQLite::Exception);
}

TEST(Database, open_existing_file_reuses) {
    const auto p = dbPath("reuse");
    {
        auto db = Database::open(p);
        db.handle().exec("CREATE TABLE keep(x INTEGER)");
        db.handle().exec("INSERT INTO keep(x) VALUES(7)");
    }
    auto db2 = Database::open(p);  // 같은 파일 재오픈
    SQLite::Statement q(db2.handle(), "SELECT x FROM keep");
    ASSERT_TRUE(q.executeStep());
    EXPECT_EQ(q.getColumn(0).getInt(), 7);
}

TEST(Database, raii_releases_connection_on_scope_exit) {
    const auto p = dbPath("raii");
    {
        auto db = Database::open(p);
        db.handle().exec("CREATE TABLE a(x INTEGER)");
    }  // 스코프 종료 → 연결 해제(소멸자)
    // 해제되었으므로 파일을 제거할 수 있고, 새 연결로 다시 열 수 있다.
    auto db2 = Database::open(p);
    EXPECT_NO_THROW(db2.handle().exec("INSERT INTO a(x) VALUES(1)"));
}
