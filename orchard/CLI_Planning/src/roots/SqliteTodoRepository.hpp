#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "trunk/ports/TodoRepository.hpp"

namespace SQLite {
class Database;
}

namespace planning::adapter_sqlite {

class SqliteTodoRepository : public ports::TodoRepository {
public:
    explicit SqliteTodoRepository(SQLite::Database& db);

    std::optional<domain::Todo> findById(domain::Todo::Id) const override;
    std::vector<domain::Todo> findByDueDate(
        std::chrono::sys_days) const override;
    std::vector<domain::Todo> findOverdue(
        std::chrono::sys_days today) const override;
    std::vector<domain::Todo> findByTag(const std::string&) const override;
    std::vector<domain::Todo> findByPriority(domain::Priority) const override;
    std::vector<domain::Todo> findAll() const override;
    void save(const domain::Todo&) override;
    void update(const domain::Todo&) override;
    void remove(domain::Todo::Id) override;

private:
    SQLite::Database& db_;
};

}  // namespace planning::adapter_sqlite
