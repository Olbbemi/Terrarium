#pragma once

#include <filesystem>

namespace SQLite {
class Database;
}

namespace planning::adapter_sqlite {

// seasons/ 의 NNN_*.sql 마이그레이션을 버전 순으로 적용한다.
// 이미 적용된 버전은 건너뛰고, 각 마이그레이션은 트랜잭션으로 적용한다.
class MigrationRunner {
public:
    explicit MigrationRunner(SQLite::Database& db);

    // 실패 시 해당 마이그레이션은 롤백되고 예외(SQLite::Exception)가 전파된다.
    void run(const std::filesystem::path& migrationsDir);

private:
    SQLite::Database& db_;
};

}  // namespace planning::adapter_sqlite
