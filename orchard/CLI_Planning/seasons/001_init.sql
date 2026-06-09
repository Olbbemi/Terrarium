-- 001_init: 초기 스키마.
-- 시간(Event): unix epoch seconds(UTC) INTEGER. 날짜(due/period): epoch days INTEGER.

CREATE TABLE IF NOT EXISTS events (
    id          TEXT    PRIMARY KEY,
    title       TEXT    NOT NULL,
    start_ts    INTEGER NOT NULL,
    end_ts      INTEGER,                       -- NULL = 종료 없음(점/하루종일)
    all_day     INTEGER NOT NULL DEFAULT 0,
    recur_freq  TEXT,                          -- NULL=비반복, daily/weekly/monthly/yearly
    recur_until INTEGER                        -- NULL=무한
);
CREATE INDEX IF NOT EXISTS idx_events_start ON events(start_ts);
CREATE INDEX IF NOT EXISTS idx_events_end   ON events(end_ts);

CREATE TABLE IF NOT EXISTS todos (
    id        TEXT    PRIMARY KEY,
    title     TEXT    NOT NULL,
    done      INTEGER NOT NULL DEFAULT 0,
    priority  INTEGER NOT NULL CHECK (priority IN (0, 1, 2)),  -- 0=HIGH,1=MEDIUM,2=LOW
    due_date  INTEGER                          -- epoch days, NULL=없음
);
CREATE INDEX IF NOT EXISTS idx_todos_due      ON todos(due_date);
CREATE INDEX IF NOT EXISTS idx_todos_priority ON todos(priority);

CREATE TABLE IF NOT EXISTS todo_tags (
    todo_id TEXT NOT NULL,
    tag     TEXT NOT NULL,
    PRIMARY KEY (todo_id, tag),
    FOREIGN KEY (todo_id) REFERENCES todos(id) ON DELETE CASCADE
);
CREATE INDEX IF NOT EXISTS idx_todo_tags_tag ON todo_tags(tag);

CREATE TABLE IF NOT EXISTS goals (
    id            TEXT    PRIMARY KEY,
    name          TEXT    NOT NULL UNIQUE,
    target_value  INTEGER NOT NULL,
    current_value INTEGER NOT NULL DEFAULT 0,
    unit          TEXT    NOT NULL,
    period_start  INTEGER NOT NULL,            -- epoch days
    period_end    INTEGER NOT NULL             -- epoch days
);
