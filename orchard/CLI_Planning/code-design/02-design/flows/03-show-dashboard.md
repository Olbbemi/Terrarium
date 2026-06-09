# 03. Dashboard (인자 없이 실행, F8)

**UseCase:** `ShowDashboardUseCase`

사용자가 인자 없이 `gaia` 실행 → 오늘 일정 개수 + 마감 임박 Todo 개수 자동 표시.

```mermaid
sequenceDiagram
    actor User
    participant CLI as adapter_cli
    participant Disp as Dispatcher
    participant UC as ShowDashboardUseCase
    participant ER as EventRepository
    participant TR as TodoRepository

    User->>CLI: gaia (인자 없이)
    CLI->>Disp: 인자 없음 감지
    Disp->>UC: execute()
    UC->>ER: findInRange(today, today)
    ER-->>UC: [오늘 Event 목록]
    UC->>TR: findOverdue(today) + 마감 임박
    TR-->>UC: [임박 Todo 목록]
    UC-->>Disp: Dashboard 데이터
    Disp->>User: "오늘 일정 N개 / 마감 임박 Todo M개"
```

**핵심 단계:**
- 진입점에서 argv 가 비어있으면 자동 Dashboard 진입 (CLI11 의 fallback subcommand 활용)
- 임박 기준 정의 필요 (예: 오늘부터 N일 이내) — 구현 단계에서 결정 또는 설정 파일로 노출
- Repository 두 개 호출 (Event + Todo) — 둘 다 인덱스 활용 (NF1 대응)
