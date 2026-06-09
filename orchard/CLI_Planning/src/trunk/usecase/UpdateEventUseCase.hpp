#pragma once

#include "trunk/domain/ConflictDetector.hpp"
#include "trunk/usecase/commands/EventCommands.hpp"
#include "trunk/ports/ConflictPrompter.hpp"
#include "trunk/ports/EventRepository.hpp"
#include "toolshed/log/Logger.hpp"

namespace planning::application {

// 이벤트 부분 수정. 시간 관련 필드가 바뀐 경우에만 충돌을 재검사한다.
class UpdateEventUseCase {
public:
    UpdateEventUseCase(ports::EventRepository& events,
                       const domain::ConflictDetector& detector,
                       ports::ConflictPrompter& prompter,
                       toolshed::log::Logger& logger);

    struct Result {
        bool cancelledByUser;
    };

    // 대상 이벤트가 없으면 std::out_of_range.
    Result execute(const UpdateEventCommand& cmd);

private:
    ports::EventRepository& events_;
    const domain::ConflictDetector& detector_;
    ports::ConflictPrompter& prompter_;
    toolshed::log::Logger& logger_;
};

}  // namespace planning::application
