#include "trunk/usecase/CreateEventUseCase.hpp"

#include "trunk/domain/TimeRange.hpp"

namespace planning::application {

CreateEventUseCase::CreateEventUseCase(ports::EventRepository& events,
                                       const domain::ConflictDetector& detector,
                                       domain::IdGenerator& idGen,
                                       toolshed::log::Logger& logger)
    : events_(events),
      detector_(detector),
      idGen_(idGen),
      logger_(logger) {}

CreateEventResult CreateEventUseCase::execute(CreateEventCommand cmd,
                                              bool force) {
    // 잘못된 시간 구간은 TimeRange 생성자가 std::invalid_argument 로 거른다.
    domain::TimeRange range(cmd.start, cmd.end, cmd.allDay);

    if (!force) {
        const auto overlapping = events_.findOverlapping(range);
        if (const auto conflict = detector_.detect(range, overlapping)) {
            // 충돌은 예외가 아닌 정상 분기. 미저장·미기록으로 반환.
            return CreateEventResult{*conflict, std::nullopt};
        }
    }

    const auto id = idGen_.next();
    // 빈 제목은 Event 생성자가 std::invalid_argument 로 거른다.
    domain::Event event(id, cmd.title, range, cmd.recurrence);
    events_.save(event);
    logger_.audit("event.create", force ? "created (forced)" : "created");
    return CreateEventResult{std::nullopt, id};
}

}  // namespace planning::application
