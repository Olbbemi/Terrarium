#pragma once

#include <chrono>
#include <optional>

namespace planning::domain {

// 이벤트의 시간 구간을 표현하는 불변 값 객체(Value Object).
// 겹침은 반열린 구간 [start, end) 의미로 판정한다.
class TimeRange {
public:
    // end 가 있을 때 start <= end 불변식을 검증한다.
    // 위반 시 std::invalid_argument 를 던진다.
    TimeRange(std::chrono::sys_seconds start,
              std::optional<std::chrono::sys_seconds> end,
              bool allDay = false);

    std::chrono::sys_seconds start() const;
    std::optional<std::chrono::sys_seconds> end() const;
    bool isAllDay() const;

    // 두 구간이 시간상 겹치면 true.
    // 반열린 구간이므로 맞닿은 경계(앞 end == 뒤 start)는 겹치지 않는다.
    bool overlaps(const TimeRange& other) const;

private:
    std::chrono::sys_seconds start_;
    std::optional<std::chrono::sys_seconds> end_;
    bool allDay_;

    // 겹침 계산용 유효 종료 시점. end 가 없으면 start(점 구간)로 본다.
    std::chrono::sys_seconds effectiveEnd() const;
};

}  // namespace planning::domain
