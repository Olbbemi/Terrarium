#pragma once

#include <chrono>
#include <optional>
#include <vector>

#include "trunk/ports/EventRepository.hpp"

namespace SQLite {
class Database;
}
namespace toolshed::sqlite {
class Database;
}

namespace planning::store {

class SqliteEventRepository : public ports::EventRepository {
public:
    // 연결은 toolshed/sqlite 경계 경유(A4b). 내부 SQL 은 handle() 로 직접 수행.
    explicit SqliteEventRepository(toolshed::sqlite::Database& db);

    std::optional<domain::Event> findById(domain::Event::Id) const override;
    std::vector<domain::Event> findOverlapping(
        const domain::TimeRange&) const override;
    std::vector<domain::Event> findInRange(
        std::chrono::sys_seconds start,
        std::chrono::sys_seconds end) const override;
    std::vector<domain::Event> findAll() const override;
    void save(const domain::Event&) override;
    void update(const domain::Event&) override;
    void remove(domain::Event::Id) override;

private:
    SQLite::Database& db_;
};

}  // namespace planning::store
