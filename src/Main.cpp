#include "LogDisplay.hpp"
#include "MenuBar.hpp"

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

    auto* logDisplay = new LogDisplay(0, 25, window.w(), window.h() - 25);
    const std::string content = readFile("pan-tadeusz.txt");
    logDisplay->setData(content.c_str(), content.size());

    auto* menu = new MenuBar();

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
