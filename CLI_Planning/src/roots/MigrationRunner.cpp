#include "roots/MigrationRunner.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <SQLiteCpp/SQLiteCpp.h>

namespace planning::adapter_sqlite {

namespace {

// 파일명 앞쪽 연속 숫자를 버전으로 해석. "001_init.sql" → 1.
int leadingVersion(const std::string& filename) {
    std::string digits;
    for (char c : filename) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            digits += c;
        } else {
            break;
        }
    }
    return digits.empty() ? -1 : std::stoi(digits);
}

std::string readFile(const std::filesystem::path& p) {
    std::ifstream in(p);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

}  // namespace

MigrationRunner::MigrationRunner(SQLite::Database& db) : db_(db) {}

void MigrationRunner::run(const std::filesystem::path& migrationsDir) {
    db_.exec(
        "CREATE TABLE IF NOT EXISTS schema_version (version INTEGER PRIMARY KEY)");

    std::vector<std::pair<int, std::filesystem::path>> migrations;
    for (const auto& entry : std::filesystem::directory_iterator(migrationsDir)) {
        if (entry.path().extension() != ".sql") continue;
        const int version = leadingVersion(entry.path().filename().string());
        if (version >= 0) migrations.emplace_back(version, entry.path());
    }
    std::sort(migrations.begin(), migrations.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    for (const auto& [version, path] : migrations) {
        SQLite::Statement check(
            db_, "SELECT COUNT(*) FROM schema_version WHERE version = ?");
        check.bind(1, version);
        check.executeStep();
        if (check.getColumn(0).getInt() > 0) continue;  // 이미 적용됨

        const std::string sql = readFile(path);
        SQLite::Transaction tx(db_);
        db_.exec(sql);  // 실패 시 예외 → 트랜잭션 소멸자가 롤백
        SQLite::Statement insert(
            db_, "INSERT INTO schema_version (version) VALUES (?)");
        insert.bind(1, version);
        insert.exec();
        tx.commit();
    }
}

}  // namespace planning::adapter_sqlite
