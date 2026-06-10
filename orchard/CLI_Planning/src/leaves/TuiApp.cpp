#include "leaves/TuiApp.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

namespace planning::ui {

TuiApp::TuiApp(UseCases usecases, toolshed::log::Logger& logger)
    : uc_(usecases), logger_(logger) {}

int TuiApp::run() {
    using namespace ftxui;

    auto screen = ScreenInteractive::Fullscreen();

    // B1 골격: 정적 플레이스홀더. B2 에서 패널(Today/Events/Todos/Goals)로 채운다.
    auto view = Renderer([] {
        return vbox({
                   text("Terrarium") | bold | hcenter,
                   separator(),
                   text("Phase B1 -- 상주 루프 골격") | hcenter,
                   filler(),
                   text("q: 종료") | dim | hcenter,
               }) |
               border;
    });

    // 종료 조건 = 사용자 q (fatal 은 FTXUI 내부에서 처리). 루프는 안 죽는다(5-4).
    auto root = CatchEvent(view, [&](const Event& event) {
        if (event == Event::Character('q')) {
            screen.Exit();
            return true;
        }
        return false;
    });

    logger_.info("tui.start");
    screen.Loop(root);
    logger_.audit("tui.session", "ended");
    return 0;
}

}  // namespace planning::ui
