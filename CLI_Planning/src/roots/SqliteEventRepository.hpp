#pragma once

#include <chrono>
#include <optional>
#include <vector>

#include "stomata/EventRepository.hpp"

namespace SQLite {
class Database;
}

namespace planning::adapter_sqlite {

class SqliteEventRepository : public ports::EventRepository {
public:
    explicit SqliteEventRepository(SQLite::Database& db);

    std::optional<domain::Event> findById(domain::Event::Id) const override;
    std::vector<domain::Event> findOverlapping(
        const domain::TimeRange&) const override;
    std::vector<domain::Event> findInRange(
        std::chrono::sys_days start, std::chrono::sys_days end) const override;
    std::vector<domain::Event> findAll() const override;
    void save(const domain::Event&) override;
    void update(const domain::Event&) override;
    void remove(domain::Event::Id) override;

private:
    SQLite::Database& db_;
};

}  // namespace planning::adapter_sqlite
