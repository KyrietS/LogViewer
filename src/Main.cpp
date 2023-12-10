#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>

int main()
{
    Fl::get_system_colors();

    Fl_Window* window = new Fl_Window(340, 180);
    Fl_Box* box = new Fl_Box(0, 0, 340, 120, "Hello, World!");
    Fl_Button* button = new Fl_Button(120, 120, 100, 30, "Close");
    button->callback([](Fl_Widget* w, void*) { w->window()->hide(); });
    button->clear_visible_focus();

    window->resizable(window);
    window->end();
    window->callback([](Fl_Widget* w, void*) {
        // Default callback will hide the windows when ESC is pressed (FL_REASON_CANCELED).
        if (Fl::callback_reason() == Fl_Callback_Reason::FL_REASON_CLOSED)
        {
            w->hide();
        }
    });
    window->show();

    return Fl::run();
}
