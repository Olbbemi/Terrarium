#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "seed/Priority.hpp"
#include "seed/Todo.hpp"

namespace planning::application {

struct AddTodoCommand {
    std::string title;
    domain::Priority priority;
    std::vector<std::string> tags;
    std::optional<std::chrono::sys_days> due;
};

// 부분 수정. 값이 있는 필드만 변경. tags 는 전체 교체.
struct UpdateTodoCommand {
    domain::Todo::Id id;
    std::optional<std::string> title;
    std::optional<domain::Priority> priority;
    std::optional<std::vector<std::string>> tags;
    std::optional<std::optional<std::chrono::sys_days>> due;
};

struct MarkTodoDoneCommand {
    domain::Todo::Id id;
};

struct DeleteTodoCommand {
    domain::Todo::Id id;
};

// 모든 필터는 선택적. 활성 필터들의 AND 로 적용.
struct ListTodosQuery {
    std::optional<std::chrono::sys_days> dueOn;        // --today (해당일 마감)
    std::optional<std::chrono::sys_days> overdueAsOf;  // --overdue (미완료 + 기한 초과)
    std::optional<std::string> tag;                    // --tag
    std::optional<domain::Priority> priority;          // --priority
};

}  // namespace planning::application
