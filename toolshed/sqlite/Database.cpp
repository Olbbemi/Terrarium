#include "toolshed/sqlite/Database.hpp"

#include <SQLiteCpp/SQLiteCpp.h>

namespace toolshed::sqlite {

Database Database::open(const std::filesystem::path& dbPath) {
    SQLite::Database db(dbPath.string(),
                        SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    // 라이프사이클 PRAGMA (v1 composition root 의 WAL/foreign_keys 를 그대로 이주,
    // busy_timeout 추가: 동시 접근 시 잠금 대기. 단일 프로세스에선 관측 불변).
    db.exec("PRAGMA journal_mode = WAL");
    db.exec("PRAGMA foreign_keys = ON");
    db.exec("PRAGMA busy_timeout = 5000");
    return Database(std::move(db));
}

}  // namespace toolshed::sqlite
