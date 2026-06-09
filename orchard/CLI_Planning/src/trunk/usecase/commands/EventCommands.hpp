#pragma once

#include <chrono>
#include <optional>
#include <string>

#include "trunk/domain/Event.hpp"
#include "trunk/domain/RecurrenceRule.hpp"

namespace planning::application {

// 신규 이벤트 생성 명령.
// (02 설계의 분리된 recurrenceUntil 은 RecurrenceRule.until 로 통합 — 정합화)
struct CreateEventCommand {
    std::string title;
    std::chrono::sys_seconds start;
    std::optional<std::chrono::sys_seconds> end;
    bool allDay = false;
    std::optional<domain::RecurrenceRule> recurrence;
};

// 이벤트 부분 수정 명령. 값이 있는 필드만 변경.
struct UpdateEventCommand {
    domain::Event::Id id;
    std::optional<std::string> title;
    std::optional<std::chrono::sys_seconds> start;
    std::optional<std::optional<std::chrono::sys_seconds>> end;  // 명시적 변경/해제 구분
    std::optional<bool> allDay;
    std::optional<std::optional<domain::RecurrenceRule>> recurrence;
};

struct DeleteEventCommand {
    domain::Event::Id id;
};

// Today/Week 등의 로컬 달력 계산은 엣지(glass/leaves)가 수행해 UTC instant
// 경계로 변환한 뒤 넘긴다. (sys_days 는 UTC 자정만 표현 가능해 로컬 하루를 못 담음.)
struct ListEventsQuery {
    std::chrono::sys_seconds rangeStart;  // 포함
    std::chrono::sys_seconds rangeEnd;    // 제외
};

}  // namespace planning::application
