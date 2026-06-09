#pragma once

#include "trunk/domain/ConflictDetector.hpp"

namespace planning::ports {

// 일정 충돌 시 사용자에게 [추가/취소] 를 묻는 인터페이스.
// CLI 외 어댑터(GUI 다이얼로그 등)로 교체 가능.
class ConflictPrompter {
public:
    enum class Choice { ADD_ANYWAY, CANCEL };

    virtual ~ConflictPrompter() = default;
    virtual Choice promptOnConflict(const domain::Conflict&) = 0;
};

}  // namespace planning::ports
