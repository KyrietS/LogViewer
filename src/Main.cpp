#include "widgets/LogDisplayWidget.hpp"
#include "widgets/MenuBarWidget.hpp"
#include "widgets/StatusBarWidget.hpp"

#include <FL/Fl.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Window.H>
#include <cassert>
#include <fstream>
#include <string>

std::string readFile(const std::string& filename)
{
    std::ifstream file(filename);
    assert(file.is_open());
    const std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content;
}

int main()
{
    Fl::get_system_colors();

    // TODO: Move window to a separate class.
    Fl_Window window(600, 500);

    const auto* menu = new MenuBarWidget(0, 0, window.w(), 25);
    const auto* statusBar = new StatusBarWidget(0, window.h() - 25, window.w(), 25);

    auto* logDisplay = new LogDisplayWidget(0, menu->h(), window.w(), window.h() - menu->h() - statusBar->h());
    const std::string content = readFile("pan-tadeusz.txt");
    logDisplay->setData(content.c_str(), content.size());

    // Center the window on the screen.
    window.position((Fl::w() - window.w()) / 2, (Fl::h() - window.h()) / 2);
    window.resizable(logDisplay);
    window.end();
    window.callback([](Fl_Widget* w, void*) {
        // Default callback will hide the windows when ESC is pressed (FL_REASON_CANCELED).
        if (Fl::callback_reason() == FL_REASON_CLOSED)
        {
            w->hide();
        }
    });
    window.show();

    return Fl::run();
}
