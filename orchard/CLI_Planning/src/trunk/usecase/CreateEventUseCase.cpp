#include "trunk/usecase/CreateEventUseCase.hpp"

#include "trunk/domain/TimeRange.hpp"

namespace planning::application {

CreateEventUseCase::CreateEventUseCase(ports::EventRepository& events,
                                       const domain::ConflictDetector& detector,
                                       domain::IdGenerator& idGen,
                                       ports::ConflictPrompter& prompter,
                                       toolshed::log::Logger& logger)
    : events_(events),
      detector_(detector),
      idGen_(idGen),
      prompter_(prompter),
      logger_(logger) {}

CreateEventUseCase::Result CreateEventUseCase::execute(
    const CreateEventCommand& cmd) {
    // 잘못된 시간 구간은 TimeRange 생성자가 std::invalid_argument 로 거른다.
    domain::TimeRange range(cmd.start, cmd.end, cmd.allDay);

    const auto overlapping = events_.findOverlapping(range);
    if (const auto conflict = detector_.detect(range, overlapping)) {
        if (prompter_.promptOnConflict(*conflict) ==
            ports::ConflictPrompter::Choice::CANCEL) {
            logger_.audit("event.create", "cancelled by user due to conflict");
            return Result{std::nullopt, true};
        }
    }

    const auto id = idGen_.next();
    domain::Event event(id, cmd.title, range, cmd.recurrence);
    events_.save(event);
    logger_.audit("event.create", "created");
    return Result{id, false};
}

}  // namespace planning::application
