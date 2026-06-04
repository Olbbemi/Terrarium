#pragma once

#include "uuid.h"

namespace planning::domain {

// 도메인 식별자(UUID) 생성 추상. 구현은 어댑터/조립부에서 제공.
class IdGenerator {
public:
    virtual ~IdGenerator() = default;
    virtual uuids::uuid next() = 0;
};

}  // namespace planning::domain
