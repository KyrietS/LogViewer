#include "FL/Fl_Browser.H"
#include "FL/Fl_File_Browser.H"
#include "FL/Fl_File_Chooser.H"
#include "FL/Fl_Free.H"
#include "FL/Fl_Native_File_Chooser.H"
#include "FL/Fl_Text_Display.H"
#include "FL/fl_ask.H"
#include "LogDisplay.hpp"

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>
#include <cassert>
#include <fstream>
#include <string>

auto* buffer = new Fl_Text_Buffer();
std::string fileContent;

int main()
{
    Fl::get_system_colors();

    auto* window = new Fl_Window(600, 500);

    auto* logDisplay = new LogDisplay(0, 0, window->w(), 240);

    std::ifstream file("pan-tadeusz.txt");
    assert(file.is_open());
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    fileContent = std::move(content);

    // const char* data = "yyyyyyyyyy\niiTTiiiiiiiii\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n";
    // logDisplay->setData(data, strlen(data));
    const char* data = fileContent.c_str();
    logDisplay->setData(data, fileContent.size());

    auto* textDisplay = new Fl_Text_Display(0, 250, window->w(), 240);
    textDisplay->buffer(buffer);
    // textDisplay->buffer()->text("Hello, World!\nHello, World!\nHello, World!\nHello, World!\nHello, "
    //                             "World!\nx\nx\nx\nx\nx\nx\nx\nx\nx\nx\nx\nx\nx\nx\nx\nx\nx\nx\nx\nx\nx\n");
    textDisplay->buffer()->text(fileContent.c_str());

    const auto button = new Fl_Button(120, 120, 100, 30, "Close");
    button->callback([](Fl_Widget* w, void*) { w->window()->hide(); });
    button->clear_visible_focus();
    button->tooltip("Click to close the window.");

    // Center the window on the screen.
    window->position((Fl::w() - window->w()) / 2, (Fl::h() - window->h()) / 2);
    window->resizable(window);
    window->end();
    window->callback([](Fl_Widget* w, void*) {
        // Default callback will hide the windows when ESC is pressed (FL_REASON_CANCELED).
        if (Fl::callback_reason() == FL_REASON_CLOSED)
        {
            w->hide();
        }
    });
    window->show();

    // auto* chooser = new Fl_Native_File_Chooser();
    // chooser->title("Choose a file");
    // chooser->show();

    return Fl::run();
}
