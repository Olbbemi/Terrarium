#pragma once

#include <vector>

#include "trunk/domain/Event.hpp"
#include "trunk/usecase/commands/EventCommands.hpp"
#include "trunk/ports/EventRepository.hpp"
#include "trunk/ports/Logger.hpp"

namespace planning::application {

// 범위 내 이벤트 조회. 반복 이벤트는 occurrence 들로 전개해 포함한다.
// 결과는 시작 시각 오름차순.
class ListEventsUseCase {
public:
    ListEventsUseCase(ports::EventRepository& events, ports::Logger& logger);

    std::vector<domain::Event> execute(const ListEventsQuery& query);

private:
    ports::EventRepository& events_;
    ports::Logger& logger_;
};

}  // namespace planning::application
