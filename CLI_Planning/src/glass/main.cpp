#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

#include <CLI/CLI.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

#include "climate/TomlConfigLoader.hpp"
#include "leaves/CliConflictPrompter.hpp"
#include "rings/SpdlogLogger.hpp"
#include "roots/MigrationRunner.hpp"
#include "roots/SqliteEventRepository.hpp"
#include "roots/SqliteTodoRepository.hpp"
#include "seed/ConflictDetector.hpp"
#include "seed/StdUuidGenerator.hpp"
#include "stem/CreateEventUseCase.hpp"
#include "stem/ShowDashboardUseCase.hpp"
#include "stem/commands/DashboardCommands.hpp"
#include "stem/commands/EventCommands.hpp"

namespace {

namespace pa = planning::application;

// 로컬 civil(broken-down) 시각 → UTC sys_seconds.
// mktime 이 시스템 로컬 타임존 기준으로 해석해 epoch 초로 변환한다.
// (GCC 11 libstdc++ 는 std::chrono::current_zone 미지원 → POSIX <ctime> 사용.)
std::chrono::sys_seconds localCivilToUtc(int y, int mo, int d, int h, int mi) {
    std::tm tm{};
    tm.tm_year = y - 1900;
    tm.tm_mon = mo - 1;
    tm.tm_mday = d;
    tm.tm_hour = h;
    tm.tm_min = mi;
    tm.tm_sec = 0;
    tm.tm_isdst = -1;  // DST 여부 자동 판단
    const std::time_t t = std::mktime(&tm);
    return std::chrono::sys_seconds{
        std::chrono::seconds{static_cast<std::int64_t>(t)}};
}

// "YYYY-MM-DDTHH:MM" 를 로컬 시각으로 해석해 UTC sys_seconds 로 저장(정책 A).
std::chrono::sys_seconds parseDateTime(const std::string& s) {
    int y = 0, mo = 0, d = 0, h = 0, mi = 0;
    if (std::sscanf(s.c_str(), "%d-%d-%dT%d:%d", &y, &mo, &d, &h, &mi) != 5) {
        throw std::runtime_error("잘못된 날짜시간 형식 (YYYY-MM-DDTHH:MM): " + s);
    }
    return localCivilToUtc(y, mo, d, h, mi);
}

}  // namespace

int main(int argc, char** argv) {
    CLI::App app{"Terrarium (gaia) — CLI 일정 관리 (vertical slice)"};
    app.require_subcommand(0);  // 서브커맨드 없으면 대시보드

    std::string configPath;
    app.add_option("--config", configPath, "TOML 설정 파일 경로")->required();

    auto* event = app.add_subcommand("event", "이벤트 관리");
    auto* eventAdd = event->add_subcommand("add", "이벤트 추가");
    std::string title, startStr, endStr;
    eventAdd->add_option("--title", title, "제목")->required();
    eventAdd->add_option("--start", startStr, "시작 (YYYY-MM-DDTHH:MM)")->required();
    eventAdd->add_option("--end", endStr, "종료 (YYYY-MM-DDTHH:MM)");

    CLI11_PARSE(app, argc, argv);

    try {
        // --- Composition Root ---
        planning::adapter_config::TomlConfigLoader config(configPath);
        planning::adapter_logger::SpdlogLogger logger(config.logConfig());

        SQLite::Database db(config.dbPath().string(),
                            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        db.exec("PRAGMA journal_mode = WAL");
        db.exec("PRAGMA foreign_keys = ON");
        planning::adapter_sqlite::MigrationRunner(db).run(TERRARIUM_MIGRATIONS_DIR);

        planning::adapter_sqlite::SqliteEventRepository eventRepo(db);
        planning::adapter_sqlite::SqliteTodoRepository todoRepo(db);
        planning::domain::ConflictDetector detector;
        planning::domain::StdUuidGenerator idGen;
        planning::adapter_cli::CliConflictPrompter prompter(std::cin, std::cout);

        pa::CreateEventUseCase createEvent(eventRepo, detector, idGen, prompter,
                                           logger);
        pa::ShowDashboardUseCase dashboard(eventRepo, todoRepo, logger);

        if (eventAdd->parsed()) {
            pa::CreateEventCommand cmd;
            cmd.title = title;
            cmd.start = parseDateTime(startStr);
            if (!endStr.empty()) cmd.end = parseDateTime(endStr);
            cmd.allDay = false;
            const auto result = createEvent.execute(cmd);
            if (result.cancelledByUser) {
                std::cout << "취소되었습니다.\n";
            } else {
                std::cout << "Event '" << title << "' 추가 완료\n";
            }
        } else if (event->parsed()) {
            std::cout << event->help();
        } else {
            // "오늘" 을 시스템 로컬 타임존 기준으로 계산(정책 A).
            const std::time_t nowT = std::time(nullptr);
            std::tm lt{};
            ::localtime_r(&nowT, &lt);
            const int ly = lt.tm_year + 1900;
            const int lmo = lt.tm_mon + 1;
            const int ld = lt.tm_mday;

            pa::ShowDashboardQuery q;
            // 로컬 달력 날짜 (todo overdue 비교용).
            q.todayDate = std::chrono::sys_days{
                std::chrono::year{ly} /
                std::chrono::month{static_cast<unsigned>(lmo)} /
                std::chrono::day{static_cast<unsigned>(ld)}};
            // 로컬 [오늘 자정, 내일 자정) 을 UTC instant 로 (event 범위용).
            // mktime 이 mday+1 을 정규화하므로 월말도 안전.
            q.dayStart = localCivilToUtc(ly, lmo, ld, 0, 0);
            q.dayEnd = localCivilToUtc(ly, lmo, ld + 1, 0, 0);

            const auto r = dashboard.execute(q);
            std::cout << "오늘 일정 " << r.todayEventsCount
                      << "개 / 마감 임박 Todo " << r.overdueTodosCount << "개\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "오류: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
