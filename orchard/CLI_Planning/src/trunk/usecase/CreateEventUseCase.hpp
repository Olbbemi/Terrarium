#pragma once

#include <optional>

#include "trunk/domain/ConflictDetector.hpp"
#include "trunk/domain/Event.hpp"
#include "trunk/domain/IdGenerator.hpp"
#include "trunk/usecase/commands/EventCommands.hpp"
#include "trunk/ports/ConflictPrompter.hpp"
#include "trunk/ports/EventRepository.hpp"
#include "toolshed/log/Logger.hpp"

namespace planning::application {

// 신규 이벤트 생성. 충돌 시 ConflictPrompter 로 사용자 선택을 위임한다.
class CreateEventUseCase {
public:
    CreateEventUseCase(ports::EventRepository& events,
                       const domain::ConflictDetector& detector,
                       domain::IdGenerator& idGen,
                       ports::ConflictPrompter& prompter,
                       toolshed::log::Logger& logger);

    struct Result {
        std::optional<domain::Event::Id> createdId;
        bool cancelledByUser;
    };

    Result execute(const CreateEventCommand& cmd);

private:
    ports::EventRepository& events_;
    const domain::ConflictDetector& detector_;
    domain::IdGenerator& idGen_;
    ports::ConflictPrompter& prompter_;
    toolshed::log::Logger& logger_;
};

}  // namespace planning::application
