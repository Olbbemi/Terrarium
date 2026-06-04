#pragma once

#include <iosfwd>

#include "stomata/ConflictPrompter.hpp"

namespace planning::adapter_cli {

// stdin/stdout 으로 충돌 시 [추가/취소] 를 묻는다.
// EOF/비대화 입력은 안전하게 취소로 처리한다.
class CliConflictPrompter : public ports::ConflictPrompter {
public:
    CliConflictPrompter(std::istream& in, std::ostream& out);
    Choice promptOnConflict(const domain::Conflict&) override;

private:
    std::istream& in_;
    std::ostream& out_;
};

}  // namespace planning::adapter_cli
