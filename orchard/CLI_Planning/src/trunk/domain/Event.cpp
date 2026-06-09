#include "trunk/domain/Event.hpp"

#include <stdexcept>
#include <utility>

namespace planning::domain {

namespace {

std::string validateTitle(std::string title) {
    if (title.empty()) {
        throw std::invalid_argument("Event: title must not be empty");
    }
    return title;
}

}  // namespace

Event::Event(Id id, std::string title, TimeRange range,
             std::optional<RecurrenceRule> recurrence)
    : id_(id),
      title_(validateTitle(std::move(title))),
      range_(range),
      recurrence_(std::move(recurrence)) {}

Event::Id Event::id() const { return id_; }

const std::string& Event::title() const { return title_; }

const TimeRange& Event::timeRange() const { return range_; }

std::optional<RecurrenceRule> Event::recurrenceRule() const { return recurrence_; }

void Event::reschedule(TimeRange newRange) { range_ = newRange; }

void Event::rename(std::string newTitle) {
    title_ = validateTitle(std::move(newTitle));
}

void Event::setRecurrence(std::optional<RecurrenceRule> rule) {
    recurrence_ = std::move(rule);
}

}  // namespace planning::domain
