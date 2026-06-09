#pragma once

#include <optional>
#include <string>
#include <vector>

#include "seed/Goal.hpp"

namespace planning::ports {

class GoalRepository {
public:
    virtual ~GoalRepository() = default;

    virtual std::optional<domain::Goal> findById(domain::Goal::Id) const = 0;
    virtual std::optional<domain::Goal> findByName(const std::string&) const = 0;
    virtual std::vector<domain::Goal> findAll() const = 0;
    virtual void save(const domain::Goal&) = 0;
    virtual void update(const domain::Goal&) = 0;
    virtual void remove(domain::Goal::Id) = 0;
};

}  // namespace planning::ports
