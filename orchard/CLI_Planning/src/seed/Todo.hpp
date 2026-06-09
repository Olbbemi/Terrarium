#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "uuid.h"

#include "seed/Priority.hpp"

namespace planning::domain {

// 할 일 (Aggregate Root).
class Todo {
public:
    using Id = uuids::uuid;

    // title 이 비어 있으면 std::invalid_argument 를 던진다.
    Todo(Id id, std::string title, Priority priority,
         std::vector<std::string> tags,
         std::optional<std::chrono::sys_days> due = std::nullopt);

    Id id() const;
    const std::string& title() const;
    bool isDone() const;
    Priority priority() const;
    const std::vector<std::string>& tags() const;
    std::optional<std::chrono::sys_days> dueDate() const;

    void markDone();
    void rename(std::string newTitle);  // 빈 제목이면 std::invalid_argument
    void updatePriority(Priority p);
    void addTag(std::string tag);       // 이미 있으면 추가하지 않음(멱등)
    void removeTag(const std::string& tag);
    void setDueDate(std::optional<std::chrono::sys_days> due);

private:
    Id id_;
    std::string title_;
    bool done_;
    Priority priority_;
    std::vector<std::string> tags_;
    std::optional<std::chrono::sys_days> due_;
};

}  // namespace planning::domain
