#pragma once

#include <chrono>
#include <optional>
#include <vector>

#include "seed/Event.hpp"
#include "seed/TimeRange.hpp"

namespace planning::ports {

class EventRepository {
public:
    virtual ~EventRepository() = default;

    virtual std::optional<domain::Event> findById(domain::Event::Id) const = 0;
    virtual std::vector<domain::Event> findOverlapping(
        const domain::TimeRange&) const = 0;
    virtual std::vector<domain::Event> findInRange(
        std::chrono::sys_days start, std::chrono::sys_days end) const = 0;
    virtual std::vector<domain::Event> findAll() const = 0;
    virtual void save(const domain::Event&) = 0;
    virtual void update(const domain::Event&) = 0;
    virtual void remove(domain::Event::Id) = 0;
};

}  // namespace planning::ports
