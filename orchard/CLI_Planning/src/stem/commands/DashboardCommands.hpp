#pragma once

#include <chrono>

namespace planning::application {

// 타임존 계산은 엣지(glass/leaves)가 수행해 명시적 값으로 넘긴다.
// - todayDate : 로컬 달력 날짜 (todo overdue 비교용, 날짜 단위)
// - dayStart/dayEnd : 로컬 '오늘' [자정, 다음 자정) 을 UTC instant 로 변환한 경계 (event 범위용)
struct ShowDashboardQuery {
    std::chrono::sys_days todayDate;
    std::chrono::sys_seconds dayStart;  // 포함
    std::chrono::sys_seconds dayEnd;    // 제외
};

}  // namespace planning::application
