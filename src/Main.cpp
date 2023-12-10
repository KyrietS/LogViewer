#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>

int main(int argc, char *argv[])
{
    Fl::get_system_colors();

    Fl_Window *window = new Fl_Window(340, 180);
    Fl_Box *box = new Fl_Box(0, 0, 340, 120, "Hello, World!");
    Fl_Button *button = new Fl_Button(120, 120, 100, 30, "Close");
    button->callback([](Fl_Widget *w, void *) { w->window()->hide(); });
    button->clear_visible_focus();

    window->resizable(window);
    window->end();
    window->show();

    return Fl::run();
}
