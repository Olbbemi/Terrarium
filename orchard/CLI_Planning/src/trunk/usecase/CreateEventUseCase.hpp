#pragma once

#include <optional>

#include "trunk/domain/ConflictDetector.hpp"
#include "trunk/domain/Event.hpp"
#include "trunk/domain/IdGenerator.hpp"
#include "trunk/usecase/commands/EventCommands.hpp"
#include "trunk/ports/EventRepository.hpp"
#include "toolshed/log/Logger.hpp"

namespace planning::application {

// 신규 이벤트 생성 결과 (2-phase). 셋 중 하나로 판별:
//  - conflict 설정  -> 충돌로 미저장 (정상 반환, 호출측이 force 재시도 여부 결정)
//  - createdId 설정 -> 저장됨
//  - throw          -> 입력 불변식 위반 (시간 구간/제목)
struct CreateEventResult {
    std::optional<domain::Conflict> conflict;
    std::optional<domain::Event::Id> createdId;
};

// 신규 이벤트 생성. 충돌 판정만 하고 사용자 상호작용은 호출측(UI)에 위임한다.
class CreateEventUseCase {
public:
    CreateEventUseCase(ports::EventRepository& events,
                       const domain::ConflictDetector& detector,
                       domain::IdGenerator& idGen,
                       toolshed::log::Logger& logger);

    // force=false: 충돌 시 미저장 {conflict}. force=true: 감지 생략하고 강제 저장.
    CreateEventResult execute(CreateEventCommand cmd, bool force = false);

private:
    ports::EventRepository& events_;
    const domain::ConflictDetector& detector_;
    domain::IdGenerator& idGen_;
    toolshed::log::Logger& logger_;
};

}  // namespace planning::application
