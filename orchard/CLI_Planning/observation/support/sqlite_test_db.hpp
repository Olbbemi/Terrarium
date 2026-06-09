#pragma once

#include <SQLiteCpp/SQLiteCpp.h>

#include "toolshed/sqlite/Database.hpp"
#include "toolshed/sqlite/MigrationRunner.hpp"

namespace planning::test {

// 실제 seasons/ 마이그레이션을 적용한 in-memory DB 를 만든다(raw 핸들).
// 아직 새 경계로 전환되지 않은 저장소(todo/goal) + 마이그레이션 메커니즘 테스트용.
// TERRARIUM_MIGRATIONS_DIR 는 CMake 가 주입하는 컴파일 정의(소스의 seasons/ 경로).
inline SQLite::Database makeMigratedDb() {
    SQLite::Database db(":memory:",
                        SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db.exec("PRAGMA foreign_keys = ON");
    toolshed::sqlite::MigrationRunner(db).run(TERRARIUM_MIGRATIONS_DIR);
    return db;
}

// 새 경계(toolshed::sqlite::Database)로 전환된 저장소용 — 래퍼를 반환한다.
// A4c 에서 todo/goal 까지 전환하면 raw makeMigratedDb 는 제거.
inline toolshed::sqlite::Database makeMigratedDatabase() {
    auto db = toolshed::sqlite::Database::open(":memory:");
    toolshed::sqlite::MigrationRunner(db.handle()).run(TERRARIUM_MIGRATIONS_DIR);
    return db;
}

inline bool tableExists(SQLite::Database& db, const std::string& name) {
    SQLite::Statement q(
        db, "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name = ?");
    q.bind(1, name);
    q.executeStep();
    return q.getColumn(0).getInt() > 0;
}

}  // namespace planning::test
