#pragma once

#include <optional>
#include <string>
#include <vector>

#include "stomata/GoalRepository.hpp"

namespace SQLite {
class Database;
}

namespace planning::adapter_sqlite {

class SqliteGoalRepository : public ports::GoalRepository {
public:
    explicit SqliteGoalRepository(SQLite::Database& db);

    std::optional<domain::Goal> findById(domain::Goal::Id) const override;
    std::optional<domain::Goal> findByName(const std::string&) const override;
    std::vector<domain::Goal> findAll() const override;
    void save(const domain::Goal&) override;
    void update(const domain::Goal&) override;
    void remove(domain::Goal::Id) override;

private:
    SQLite::Database& db_;
};

}  // namespace planning::adapter_sqlite
