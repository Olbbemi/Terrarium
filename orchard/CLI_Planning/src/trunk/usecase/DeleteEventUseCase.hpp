#pragma once

#include "trunk/usecase/commands/EventCommands.hpp"
#include "trunk/ports/EventRepository.hpp"
#include "toolshed/log/Logger.hpp"

namespace planning::application {

// 이벤트 삭제. 반복 이벤트는 단일 레코드라 삭제 = 모든 인스턴스 제거.
class DeleteEventUseCase {
public:
    DeleteEventUseCase(ports::EventRepository& events, toolshed::log::Logger& logger);

    // 대상 이벤트가 없으면 std::out_of_range.
    void execute(const DeleteEventCommand& cmd);

private:
    ports::EventRepository& events_;
    toolshed::log::Logger& logger_;
};

}  // namespace planning::application
