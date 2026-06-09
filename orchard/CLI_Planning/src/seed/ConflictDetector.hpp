#pragma once

#include <optional>
#include <string>
#include <vector>

#include "seed/Event.hpp"
#include "seed/TimeRange.hpp"

namespace planning::domain {

// 충돌한 기존 이벤트 정보(충돌 안내 메시지에 사용).
struct Conflict {
    Event::Id existingEventId;
    std::string existingTitle;
    TimeRange existingRange;
};

// 후보 시간 구간이 기존 이벤트들과 겹치는지 판정하는 도메인 서비스.
class ConflictDetector {
public:
    // 겹치는 첫 이벤트를 Conflict 로 반환. 겹침이 없으면 nullopt.
    std::optional<Conflict> detect(
        const TimeRange& candidate,
        const std::vector<Event>& existingOverlapping) const;
};

}  // namespace planning::domain
