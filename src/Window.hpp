#pragma once
#include "widgets/LogDisplayWidget.hpp"
#include "widgets/MenuBarWidget.hpp"
#include "widgets/StatusBarWidget.hpp"
#include <FL/Fl_Window.H>

namespace
{
constexpr int MENU_BAR_HEIGHT = 25;
constexpr int STATUS_BAR_HEIGHT = 25;
} // namespace

struct AppContext
{
    Fl_Window* window;
    MenuBarWidget* menuBar;
    StatusBarWidget* statusBar;
    LogDisplayWidget* logDisplay;
};

class Window
{
public:
    Window(int width, int height)
    {
        createWindowWidget();
        createMenuBarWidget();
        createStatusBarWidget();
        createLogDisplayWidget();

        context.window->show();
    }

    void show()
    {
        context.window->show();
    }

    void loadData(const char* data, size_t size)
    {
        context.logDisplay->setData(data, size);
        context.statusBar->setNumberOfLines(context.logDisplay->getLines().size());
        context.statusBar->setStatusInformation("File loaded successfully");
    }

private:
    void createWindowWidget()
    {
        auto window = new Fl_Window(600, 500);
        window->position((Fl::w() - window->w()) / 2, (Fl::h() - window->h()) / 2);
        window->end();
        window->callback([](Fl_Widget* w, void*) {
            if (Fl::callback_reason() == FL_REASON_CLOSED)
            {
                w->hide();
            }
        });
        window->end();
        context.window = window;
    }

    void createMenuBarWidget()
    {
        auto window = context.window;
        window->begin();
        context.menuBar = new MenuBarWidget(0, 0, context.window->w(), MENU_BAR_HEIGHT);
        window->end();
    }

    void createStatusBarWidget()
    {
        auto window = context.window;
        window->begin();
        context.statusBar = new StatusBarWidget(0, window->h() - STATUS_BAR_HEIGHT, window->w(), STATUS_BAR_HEIGHT);
        window->end();
    }

    void createLogDisplayWidget()
    {
        auto window = context.window;
        window->begin();
        context.logDisplay =
            new LogDisplayWidget(0, MENU_BAR_HEIGHT, window->w(), window->h() - MENU_BAR_HEIGHT - STATUS_BAR_HEIGHT);
        window->resizable(context.logDisplay);
        window->end();

        context.logDisplay->onCursorPositionChanged([this](size_t lineIndex, size_t charIndex) {
            auto statusBar = context.statusBar;
            auto line = context.logDisplay->getLines().at(lineIndex);
            statusBar->setCursorPosition(lineIndex + 1, charIndex, line.first + charIndex);
        });
    }

    AppContext context{};
};