#include "roots/SqliteGoalRepository.hpp"

#include <chrono>
#include <cstdint>
#include <string>

#include <SQLiteCpp/SQLiteCpp.h>

#include "toolshed/sqlite/Database.hpp"
#include "trunk/domain/Goal.hpp"

namespace planning::adapter_sqlite {

namespace {

using namespace std::chrono;

int64_t toEpochDays(sys_days d) {
    return static_cast<int64_t>(d.time_since_epoch().count());
}

sys_days fromEpochDays(int64_t v) { return sys_days{days{v}}; }

// SELECT 컬럼: id,name,target_value,current_value,unit,period_start,period_end
const char* kCols =
    "id, name, target_value, current_value, unit, period_start, period_end";

domain::Goal rowToGoal(SQLite::Statement& q) {
    auto id = uuids::uuid::from_string(q.getColumn(0).getString()).value();
    std::string name = q.getColumn(1).getString();
    const int target = q.getColumn(2).getInt();
    const int current = q.getColumn(3).getInt();
    std::string unit = q.getColumn(4).getString();
    auto ps = fromEpochDays(q.getColumn(5).getInt64());
    auto pe = fromEpochDays(q.getColumn(6).getInt64());
    return domain::Goal(id, name, target, current, unit, ps, pe);
}

}  // namespace

SqliteGoalRepository::SqliteGoalRepository(toolshed::sqlite::Database& db)
    : db_(db.handle()) {}

std::optional<domain::Goal> SqliteGoalRepository::findById(
    domain::Goal::Id id) const {
    SQLite::Statement q(
        db_, std::string("SELECT ") + kCols + " FROM goals WHERE id = ?");
    q.bind(1, uuids::to_string(id));
    if (q.executeStep()) return rowToGoal(q);
    return std::nullopt;
}

std::optional<domain::Goal> SqliteGoalRepository::findByName(
    const std::string& name) const {
    SQLite::Statement q(
        db_, std::string("SELECT ") + kCols + " FROM goals WHERE name = ?");
    q.bind(1, name);
    if (q.executeStep()) return rowToGoal(q);
    return std::nullopt;
}

std::vector<domain::Goal> SqliteGoalRepository::findAll() const {
    SQLite::Statement q(db_, std::string("SELECT ") + kCols + " FROM goals");
    std::vector<domain::Goal> out;
    while (q.executeStep()) out.push_back(rowToGoal(q));
    return out;
}

void SqliteGoalRepository::save(const domain::Goal& g) {
    SQLite::Statement s(
        db_,
        "INSERT INTO goals (id, name, target_value, current_value, unit, "
        "period_start, period_end) VALUES (?, ?, ?, ?, ?, ?, ?)");
    s.bind(1, uuids::to_string(g.id()));
    s.bind(2, g.name());
    s.bind(3, g.targetValue());
    s.bind(4, g.currentValue());
    s.bind(5, g.unit());
    s.bind(6, toEpochDays(g.periodStart()));
    s.bind(7, toEpochDays(g.periodEnd()));
    s.exec();
}

void SqliteGoalRepository::update(const domain::Goal& g) {
    SQLite::Statement s(
        db_,
        "UPDATE goals SET name = ?, target_value = ?, current_value = ?, "
        "unit = ?, period_start = ?, period_end = ? WHERE id = ?");
    s.bind(1, g.name());
    s.bind(2, g.targetValue());
    s.bind(3, g.currentValue());
    s.bind(4, g.unit());
    s.bind(5, toEpochDays(g.periodStart()));
    s.bind(6, toEpochDays(g.periodEnd()));
    s.bind(7, uuids::to_string(g.id()));
    s.exec();
}

void SqliteGoalRepository::remove(domain::Goal::Id id) {
    SQLite::Statement s(db_, "DELETE FROM goals WHERE id = ?");
    s.bind(1, uuids::to_string(id));
    s.exec();
}

}  // namespace planning::adapter_sqlite
