#pragma once
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Native_File_Chooser.H>

#include <iostream>

class MenuBarWidget : public Fl_Menu_Bar
{
public:
    MenuBarWidget(const int x, const int y, const int w, const int h) : Fl_Menu_Bar(x, y, w, h)
    {
        box(FL_FLAT_BOX);
        menu_box(FL_UP_BOX);
        buildMenu();
    }

private:
    void buildMenu()
    {
        void* noUserData = nullptr;
        Fl_Callback* noCallback = nullptr;
        constexpr int noShortcut = 0;

        add("File/@fileopen  Open File...", FL_CTRL + 'o', openFileDialog, noUserData, 0);
        add("File/@filesave  Save", FL_CTRL + 's', noCallback, noUserData, FL_MENU_INACTIVE);
        add("File/@filesaveas  Save As...", FL_CTRL + FL_SHIFT + 's', saveFileDialog, noUserData, 0);
        add("File/Close File", FL_CTRL + 'w', noCallback, noUserData, FL_MENU_INACTIVE);
        add("File/Settings", noShortcut, noCallback, noUserData, FL_MENU_INACTIVE | FL_MENU_DIVIDER);
        add("File/Quit", FL_CTRL + 'q', quitCallback);
        add("_Search", noShortcut, noCallback, noUserData, FL_SUBMENU /* | FL_MENU_INACTIVE */);
        add("Search/Find", FL_CTRL + 'f', noCallback, noUserData, FL_MENU_INACTIVE);
        add("Search/Find All     ", FL_CTRL + FL_SHIFT + 'f', noCallback, noUserData, FL_MENU_INACTIVE);
        add("Search/Filter     ", FL_CTRL + 'g', noCallback, noUserData, FL_MENU_INACTIVE);
        add("Help/About    ", FL_F + 1, noCallback, noUserData, FL_MENU_INACTIVE);
        global();
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
};