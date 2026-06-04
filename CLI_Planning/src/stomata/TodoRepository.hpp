#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "seed/Priority.hpp"
#include "seed/Todo.hpp"

namespace planning::ports {

class TodoRepository {
public:
    virtual ~TodoRepository() = default;

    virtual std::optional<domain::Todo> findById(domain::Todo::Id) const = 0;
    virtual std::vector<domain::Todo> findByDueDate(
        std::chrono::sys_days) const = 0;
    virtual std::vector<domain::Todo> findOverdue(
        std::chrono::sys_days today) const = 0;
    virtual std::vector<domain::Todo> findByTag(const std::string&) const = 0;
    virtual std::vector<domain::Todo> findByPriority(domain::Priority) const = 0;
    virtual std::vector<domain::Todo> findAll() const = 0;
    virtual void save(const domain::Todo&) = 0;
    virtual void update(const domain::Todo&) = 0;
    virtual void remove(domain::Todo::Id) = 0;
};

}  // namespace planning::ports
