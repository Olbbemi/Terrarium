#pragma once

#include <optional>

#include "trunk/domain/ConflictDetector.hpp"
#include "trunk/usecase/commands/EventCommands.hpp"
#include "trunk/ports/EventRepository.hpp"
#include "toolshed/log/Logger.hpp"

namespace planning::application {

// 이벤트 수정 결과 (2-phase). 셋 중 하나로 판별:
//  - conflict 설정 -> 충돌로 미저장 (정상 반환, 호출측이 force 재시도 결정)
//  - updated=true  -> 저장됨
//  - throw         -> 대상 없음(out_of_range) / 시간 구간 위반(invalid_argument)
struct UpdateEventResult {
    std::optional<domain::Conflict> conflict;
    bool updated = false;
};

// 이벤트 부분 수정. 시간 관련 필드가 바뀐 경우에만 충돌을 재검사한다.
// 충돌 판정만 하고 사용자 상호작용은 호출측(UI)에 위임한다.
class UpdateEventUseCase {
public:
    UpdateEventUseCase(ports::EventRepository& events,
                       const domain::ConflictDetector& detector,
                       toolshed::log::Logger& logger);

    // force=false: 시간 변경 시 충돌이면 미저장 {conflict}.
    // force=true: 재검사 생략하고 강제 저장.
    UpdateEventResult execute(UpdateEventCommand cmd, bool force = false);

private:
    ports::EventRepository& events_;
    const domain::ConflictDetector& detector_;
    toolshed::log::Logger& logger_;
};

}  // namespace planning::application
