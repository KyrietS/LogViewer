#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Native_File_Chooser.H>

#include <cassert>
#include <iostream>
#include <memory>

class MenuBar
{
  public:
    MenuBar()
    {
        const auto* window = Fl_Group::current();
        assert(window != nullptr && "No current window");

        menu = new Fl_Menu_Bar(0, 0, window->w(), 25);
        menu->box(FL_FLAT_BOX);
        menu->menu_box(FL_UP_BOX);
        buildMenu();
    }

  private:
    void buildMenu() const
    {
        void* noUserData = nullptr;
        Fl_Callback* noCallback = nullptr;
        constexpr int noShortcut = 0;

        menu->add("File/@fileopen  Open File...", FL_CTRL + 'o', openFileDialog, noUserData, 0);
        menu->add("File/@filesave  Save", FL_CTRL + 's', noCallback, noUserData, FL_MENU_INACTIVE);
        menu->add("File/@filesaveas  Save As...", FL_CTRL + FL_SHIFT + 's', saveFileDialog, noUserData, 0);
        menu->add("File/Close File", FL_CTRL + 'w', noCallback, noUserData, FL_MENU_INACTIVE);
        menu->add("File/Settings", noShortcut, noCallback, noUserData, FL_MENU_INACTIVE | FL_MENU_DIVIDER);
        menu->add("File/Quit", FL_CTRL + 'q', quitCallback);
        menu->add("Search", noShortcut, noCallback, noUserData, FL_SUBMENU /* | FL_MENU_INACTIVE */);
        menu->add("Search/Find", FL_CTRL + 'f', noCallback, noUserData, FL_MENU_INACTIVE);
        menu->add("Search/Find All     ", FL_CTRL + FL_SHIFT + 'f', noCallback, noUserData, FL_MENU_INACTIVE);
        menu->add("Search/Filter     ", FL_CTRL + 'g', noCallback, noUserData, FL_MENU_INACTIVE);
        menu->add("Help/About    ", FL_F + 1, noCallback, noUserData, FL_MENU_INACTIVE);
        menu->global();
    }

    static void quitCallback(Fl_Widget*, void*)
    {
        exit(0);
    }

    static void openFileDialog(Fl_Widget*, void*)
    {
        Fl_Native_File_Chooser fileChooser;
        fileChooser.title(nullptr);
        fileChooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
        fileChooser.filter("Log Files\t*.log\nLog Alligator Project\t*.laproj");
        switch (fileChooser.show())
        {
        case -1:
            std::cout << "File Open ERROR: " << fileChooser.errmsg() << std::endl;
            break;
        case 1:
            std::cout << "File Open CANCELLED" << std::endl;
            break;
        default:
            std::cout << "File Open OK: " << fileChooser.filename() << std::endl;
        }
    }

    static void saveFileDialog(Fl_Widget*, void*)
    {
        Fl_Native_File_Chooser fileChooser;
        fileChooser.title(nullptr);
        fileChooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
        fileChooser.options(Fl_Native_File_Chooser::SAVEAS_CONFIRM); // Confirm file overwrite.
        fileChooser.filter("Log Alligator Project\t*.laproj");
        switch (fileChooser.show())
        {
        case -1:
            std::cout << "File Save ERROR: " << fileChooser.errmsg() << std::endl;
            break;
        case 1:
            std::cout << "File Save CANCELLED" << std::endl;
            break;
        default:
            std::cout << "File Save OK: " << fileChooser.filename() << std::endl;
        }
    }

    // The memory is managed by FLTK. No need to call delete manually.
    Fl_Menu_Bar* menu;
};