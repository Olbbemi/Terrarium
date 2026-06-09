#include "leaves/CliConflictPrompter.hpp"

#include <istream>
#include <ostream>
#include <string>

#include "trunk/domain/ConflictDetector.hpp"

namespace planning::ui {

CliConflictPrompter::CliConflictPrompter(std::istream& in, std::ostream& out)
    : in_(in), out_(out) {}

ports::ConflictPrompter::Choice CliConflictPrompter::promptOnConflict(
    const domain::Conflict& conflict) {
    out_ << "기존 '" << conflict.existingTitle
         << "' 와 시간이 겹칩니다. 그래도 추가할까요? [y/N]: ";
    out_.flush();

    std::string line;
    if (!std::getline(in_, line)) {
        return Choice::CANCEL;  // EOF/비대화 → 안전 취소
    }
    if (line == "y" || line == "Y" || line == "yes") {
        return Choice::ADD_ANYWAY;
    }
    return Choice::CANCEL;
}

}  // namespace planning::ui
