#include "trunk/usecase/DeleteEventUseCase.hpp"

#include <stdexcept>

namespace planning::application {

DeleteEventUseCase::DeleteEventUseCase(ports::EventRepository& events,
                                       toolshed::log::Logger& logger)
    : events_(events), logger_(logger) {}

void DeleteEventUseCase::execute(const DeleteEventCommand& cmd) {
    if (!events_.findById(cmd.id)) {
        throw std::out_of_range("DeleteEventUseCase: event not found");
    }
    events_.remove(cmd.id);
    logger_.audit("event.delete", "deleted");
}

}  // namespace planning::application
