#pragma once

#include <chrono>
#include <optional>
#include <vector>

#include "trunk/domain/Event.hpp"
#include "trunk/domain/TimeRange.hpp"

namespace planning::ports {

class EventRepository {
public:
    virtual ~EventRepository() = default;

    virtual std::optional<domain::Event> findById(domain::Event::Id) const = 0;
    virtual std::vector<domain::Event> findOverlapping(
        const domain::TimeRange&) const = 0;
    // [start, end) UTC instant 범위. 로컬 '오늘/이번 주' 등의 달력 계산은
    // 엣지(glass/leaves)가 수행해 UTC instant 경계로 변환한 뒤 넘긴다.
    virtual std::vector<domain::Event> findInRange(
        std::chrono::sys_seconds start, std::chrono::sys_seconds end) const = 0;
    virtual std::vector<domain::Event> findAll() const = 0;
    virtual void save(const domain::Event&) = 0;
    virtual void update(const domain::Event&) = 0;
    virtual void remove(domain::Event::Id) = 0;
};

}  // namespace planning::ports
