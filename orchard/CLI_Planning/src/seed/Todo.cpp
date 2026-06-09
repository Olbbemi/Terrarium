#include "seed/Todo.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace planning::domain {

namespace {

std::string validateTitle(std::string title) {
    if (title.empty()) {
        throw std::invalid_argument("Todo: title must not be empty");
    }
    return title;
}

}  // namespace

Todo::Todo(Id id, std::string title, Priority priority,
           std::vector<std::string> tags,
           std::optional<std::chrono::sys_days> due)
    : id_(id),
      title_(validateTitle(std::move(title))),
      done_(false),
      priority_(priority),
      tags_(std::move(tags)),
      due_(due) {}

Todo::Id Todo::id() const { return id_; }

const std::string& Todo::title() const { return title_; }

bool Todo::isDone() const { return done_; }

Priority Todo::priority() const { return priority_; }

const std::vector<std::string>& Todo::tags() const { return tags_; }

std::optional<std::chrono::sys_days> Todo::dueDate() const { return due_; }

void Todo::markDone() { done_ = true; }

void Todo::rename(std::string newTitle) {
    title_ = validateTitle(std::move(newTitle));
}

void Todo::updatePriority(Priority p) { priority_ = p; }

void Todo::addTag(std::string tag) {
    if (std::find(tags_.begin(), tags_.end(), tag) == tags_.end()) {
        tags_.push_back(std::move(tag));
    }
}

void Todo::removeTag(const std::string& tag) {
    tags_.erase(std::remove(tags_.begin(), tags_.end(), tag), tags_.end());
}

void Todo::setDueDate(std::optional<std::chrono::sys_days> due) { due_ = due; }

}  // namespace planning::domain
