#pragma once
#include "widgets/LogDisplayWidget.hpp"
#include "widgets/MenuBarWidget.hpp"
#include "widgets/SearchBarWidget.hpp"
#include "widgets/StatusBarWidget.hpp"
#include <FL/Fl_Window.H>

namespace
{
constexpr int MENU_BAR_HEIGHT = 25;
constexpr int STATUS_BAR_HEIGHT = 25;
constexpr int SEARCH_BAR_HEIGHT = 25;
} // namespace

struct AppContext
{
    Fl_Window* window;
    MenuBarWidget* menuBar;
    SearchBarWidget* searchBar;
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
        createSearchBarWidget();
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

    void createSearchBarWidget()
    {
        auto window = context.window;
        window->begin();
        context.searchBar = new SearchBarWidget(0, MENU_BAR_HEIGHT, window->w(), SEARCH_BAR_HEIGHT);
        window->end();

        context.searchBar->onSearch([this](const std::string& query) { search(query); });

        context.searchBar->onClose([this] {
            const auto window = context.window;
            context.searchBar->hide();
            context.logDisplay->resize(0, MENU_BAR_HEIGHT, window->w(),
                                       window->h() - MENU_BAR_HEIGHT - STATUS_BAR_HEIGHT);
        });
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
        constexpr int widgetTopOffset = MENU_BAR_HEIGHT + SEARCH_BAR_HEIGHT;
        const int widgetHeight = window->h() - MENU_BAR_HEIGHT - STATUS_BAR_HEIGHT - SEARCH_BAR_HEIGHT;
        context.logDisplay = new LogDisplayWidget(0, widgetTopOffset, window->w(), widgetHeight);
        window->resizable(context.logDisplay);
        window->end();

        context.logDisplay->onCursorPositionChanged([this](size_t lineIndex, size_t charIndex) {
            auto statusBar = context.statusBar;
            auto line = context.logDisplay->getLines().at(lineIndex);
            statusBar->setCursorPosition(lineIndex + 1, charIndex, line.first + charIndex);
        });
    }

    void search( const std::string& query )
    {
        auto& logDisplay = context.logDisplay;
        const auto& lines = logDisplay->getLines();
        const auto& data = logDisplay->getData();

        for (size_t lineIndex = 0; lineIndex < lines.size(); lineIndex++)
        {
            auto [lineBegin, lineEnd] = lines[lineIndex];
            std::string_view line(data + lineBegin, lineEnd - lineBegin);
            size_t pos = line.find(query);
            if (pos != std::string_view::npos)
            {
                size_t begin = lineBegin + pos;
                size_t end = begin + query.size();
                logDisplay->select(begin, end);
                logDisplay->scrollToLine(lineIndex);
                context.statusBar->setStatusInformation("Match found: " + query);
                return;
            }
        }

        context.statusBar->setStatusInformation("No matches found: " + query);
	}

    AppContext context{};
};