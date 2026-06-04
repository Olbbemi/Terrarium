#include "seed/Goal.hpp"

#include <stdexcept>
#include <utility>

namespace planning::domain {

namespace {

int validateTarget(int target) {
    if (target <= 0) {
        throw std::invalid_argument("Goal: targetValue must be positive");
    }
    return target;
}

void validatePeriod(std::chrono::sys_days start, std::chrono::sys_days end) {
    if (start > end) {
        throw std::invalid_argument("Goal: periodStart must be <= periodEnd");
    }
}

}  // namespace

Goal::Goal(Id id, std::string name, int targetValue, std::string unit,
           std::chrono::sys_days periodStart, std::chrono::sys_days periodEnd)
    : Goal(id, std::move(name), targetValue, 0, std::move(unit), periodStart,
           periodEnd) {}

Goal::Goal(Id id, std::string name, int targetValue, int currentValue,
           std::string unit, std::chrono::sys_days periodStart,
           std::chrono::sys_days periodEnd)
    : id_(id),
      name_(std::move(name)),
      targetValue_(validateTarget(targetValue)),
      currentValue_(currentValue),
      unit_(std::move(unit)),
      periodStart_(periodStart),
      periodEnd_(periodEnd) {
    validatePeriod(periodStart_, periodEnd_);
}

Goal::Id Goal::id() const { return id_; }

const std::string& Goal::name() const { return name_; }

int Goal::targetValue() const { return targetValue_; }

int Goal::currentValue() const { return currentValue_; }

const std::string& Goal::unit() const { return unit_; }

std::chrono::sys_days Goal::periodStart() const { return periodStart_; }

std::chrono::sys_days Goal::periodEnd() const { return periodEnd_; }

void Goal::incrementCounter() { ++currentValue_; }

void Goal::rename(std::string newName) { name_ = std::move(newName); }

void Goal::updateTarget(int newTarget) {
    targetValue_ = validateTarget(newTarget);
}

void Goal::updatePeriod(std::chrono::sys_days start, std::chrono::sys_days end) {
    validatePeriod(start, end);
    periodStart_ = start;
    periodEnd_ = end;
}

double Goal::progressRatio() const {
    return static_cast<double>(currentValue_) / targetValue_;
}

}  // namespace planning::domain
