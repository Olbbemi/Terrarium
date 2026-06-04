#pragma once

#include "stem/commands/EventCommands.hpp"
#include "stomata/EventRepository.hpp"
#include "stomata/Logger.hpp"

namespace planning::application {

// 이벤트 삭제. 반복 이벤트는 단일 레코드라 삭제 = 모든 인스턴스 제거.
class DeleteEventUseCase {
public:
    DeleteEventUseCase(ports::EventRepository& events, ports::Logger& logger);

    // 대상 이벤트가 없으면 std::out_of_range.
    void execute(const DeleteEventCommand& cmd);

private:
    ports::EventRepository& events_;
    ports::Logger& logger_;
};

}  // namespace planning::application
