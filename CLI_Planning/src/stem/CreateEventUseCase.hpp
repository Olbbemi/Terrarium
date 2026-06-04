#pragma once

#include <optional>

#include "seed/ConflictDetector.hpp"
#include "seed/Event.hpp"
#include "seed/IdGenerator.hpp"
#include "stem/commands/EventCommands.hpp"
#include "stomata/ConflictPrompter.hpp"
#include "stomata/EventRepository.hpp"
#include "stomata/Logger.hpp"

namespace planning::application {

// 신규 이벤트 생성. 충돌 시 ConflictPrompter 로 사용자 선택을 위임한다.
class CreateEventUseCase {
public:
    CreateEventUseCase(ports::EventRepository& events,
                       const domain::ConflictDetector& detector,
                       domain::IdGenerator& idGen,
                       ports::ConflictPrompter& prompter,
                       ports::Logger& logger);

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
    ports::Logger& logger_;
};

}  // namespace planning::application
