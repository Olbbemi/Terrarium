#pragma once

#include <filesystem>

#include <SQLiteCpp/Database.h>

namespace toolshed::sqlite {

// SQLite 연결의 RAII 래퍼 (앱 무관, 도메인-free).
// open() 이 WAL / foreign_keys / busy_timeout PRAGMA 를 일괄 적용한다.
// SQL 추상화는 하지 않는다 — handle() 로 SQLiteCpp 를 그대로 노출(store 가 직접 SQL).
class Database {
public:
    // RW|CREATE 로 열고 라이프사이클 PRAGMA 를 적용한다.
    // 실패 시 SQLite::Exception 전파.
    static Database open(const std::filesystem::path& dbPath);

    SQLite::Database& handle() { return db_; }
    const SQLite::Database& handle() const { return db_; }

private:
    explicit Database(SQLite::Database&& db) : db_(std::move(db)) {}

    SQLite::Database db_;  // 소유(move-only) — 스코프 종료 시 연결 해제
};

}  // namespace toolshed::sqlite
