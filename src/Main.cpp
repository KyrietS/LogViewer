#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>

int main()
{
    Fl::get_system_colors();

    auto* window = new Fl_Window(340, 180);
    auto box = new Fl_Box(0, 0, 340, 120, "Hello, World!");

    const auto button = new Fl_Button(120, 120, 100, 30, "Close");
    button->callback([](Fl_Widget* w, void*) { w->window()->hide(); });
    button->clear_visible_focus();
    button->tooltip("Click to close the window.");

    // Center the window on the screen.
    window->position((Fl::w() - window->w()) / 2, (Fl::h() - window->h()) / 2);
    window->resizable(nullptr);
    window->end();
    window->callback([](Fl_Widget* w, void*) {
        // Default callback will hide the windows when ESC is pressed (FL_REASON_CANCELED).
        if (Fl::callback_reason() == FL_REASON_CLOSED)
        {
            w->hide();
        }
    });
    window->show();

    return Fl::run();
}
