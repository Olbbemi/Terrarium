#pragma once

#include <SQLiteCpp/SQLiteCpp.h>

#include "roots/MigrationRunner.hpp"

namespace planning::test {

// 실제 seasons/ 마이그레이션을 적용한 in-memory DB 를 만든다.
// TERRARIUM_MIGRATIONS_DIR 는 CMake 가 주입하는 컴파일 정의(소스의 seasons/ 경로).
inline SQLite::Database makeMigratedDb() {
    SQLite::Database db(":memory:",
                        SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db.exec("PRAGMA foreign_keys = ON");
    adapter_sqlite::MigrationRunner(db).run(TERRARIUM_MIGRATIONS_DIR);
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
