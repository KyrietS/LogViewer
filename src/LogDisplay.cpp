#include "LogDisplay.hpp"

#include "FL/Fl_Window.H"
#include "FL/fl_draw.H"

#include <array>
#include <cassert>
#include <iostream>
#include <span>

namespace
{
constexpr int TOP_MARGIN = 1;
constexpr int BOTTOM_MARGIN = 1;
constexpr int LEFT_MARGIN = 3;
constexpr int RIGHT_MARGIN = 3;
} // namespace

LogDisplay::LogDisplay(int X, int Y, int W, int H, const char* l) : Fl_Group(X, Y, W, H, l)
{
    vScrollBar = new Fl_Scrollbar(0, 0, 1, 1);
    vScrollBar->callback(reinterpret_cast<Fl_Callback*>(vScrollCallback), this);

    // TODO: Move to some recalc function
    vScrollBar->value(1, 20, 1, 100);
    vScrollBar->linesize(3);
    vScrollBar->set_visible();

    textFont = FL_HELVETICA;
    textSize = FL_NORMAL_SIZE;
    textColor = FL_FOREGROUND_COLOR;

    recalcSize();

    color(FL_BACKGROUND2_COLOR, FL_SELECTION_COLOR);
    box(FL_DOWN_FRAME);
    end();
}

LogDisplay::~LogDisplay()
{
}

void LogDisplay::setData(const char* data, size_t size)
{
    this->data = data;
    this->dataSize = size;

    size_t lineStart = 0;
    size_t lineEnd = 0;
    while (lineEnd < dataSize)
    {
        while (lineEnd < dataSize && data[lineEnd] != '\n')
        {
            ++lineEnd;
        }
        lines.emplace_back(lineStart, lineEnd);
        ++lineEnd;
        lineStart = lineEnd;
    }

    vScrollBar->value(1, 20, 1, lines.size());
}

const char* LogDisplay::getData() const
{
    return data;
}

void LogDisplay::draw()
{
    std::cout << "LogDisplay::draw()" << std::endl;
    fl_push_clip(x(), y(), w(), h()); // prevent drawing outside widget area

    recalcSize();

    Fl_Color bgcolor = color();
    // fl_rectf(x(), y(), w(), h(), bgcolor);
    if (damage() & FL_DAMAGE_ALL)
    {
        fl_rectf(x(), y(), w(), h(), bgcolor);
        draw_box();
    }

    // draw the scrollbars
    if (damage() & (FL_DAMAGE_ALL))
    {
        vScrollBar->damage(FL_DAMAGE_ALL);
        std::cout << "damage: " << (int)damage() << std::endl;
    }
    if (damage() & FL_DAMAGE_SCROLL)
    {
        std::cout << "damage2: " << (int)damage() << std::endl;
        vScrollBar->damage(FL_DAMAGE_SCROLL);
    }
    update_child(*vScrollBar);

    drawText();

    fl_pop_clip();
}

int LogDisplay::handle(int event)
{
    bool dragging = false; // TODO: not implemented

    if (!Fl::event_inside(textArea.x, textArea.y, textArea.w, textArea.h) && !dragging && event != FL_LEAVE &&
        event != FL_ENTER && event != FL_MOVE && event != FL_FOCUS && event != FL_UNFOCUS && event != FL_KEYBOARD &&
        event != FL_KEYUP)
    {
        return Fl_Group::handle(event);
    }

    if (!active_r())
    {
        return 0;
    }

    switch (event)
    {
    case FL_PUSH:
        if (active_r())
        {
            if (Fl::event_inside(textArea.x, textArea.y, textArea.w, textArea.h))
            {
                bool doubleClick = Fl::event_clicks();
                if (doubleClick)
                    selectWord(Fl::event_x(), Fl::event_y());
                else
                    setSelectionStart(Fl::event_x(), Fl::event_y());
                damage(FL_DAMAGE_SCROLL);
                return 1;
            }
        }
        return 0;
    case FL_DRAG:
        if (Fl::event_inside(textArea.x, textArea.y, textArea.w, textArea.h))
        {
            setSelectionEnd(Fl::event_x(), Fl::event_y());
            damage(FL_DAMAGE_SCROLL);
            return 1;
        }
    case FL_MOUSEWHEEL:
        if (active_r())
        {
            return vScrollBar->handle(event);
            // int delta = Fl::event_dy();
            // firstLine += delta;
            // firstLine = firstLine < 0 ? 0 : firstLine;
            // vScrollBar->value(firstLine);
            // redraw();
            // return 1;
        }
        return 0;
    case FL_ENTER:
    case FL_MOVE:
        if (active_r())
        {
            if (Fl::event_inside(textArea.x, textArea.y, textArea.w, textArea.h))
            {
                window()->cursor(FL_CURSOR_INSERT);
            }
            else
            {
                window()->cursor(FL_CURSOR_DEFAULT);
            }
            return 1;
        }
        return 0;

    case FL_LEAVE:
    case FL_HIDE:
        if (active_r() && window())
        {
            window()->cursor(FL_CURSOR_DEFAULT);
            return 1;
        }
        return 0;
    }

    return 0;
}

void LogDisplay::drawText()
{
    std::cout << "LogDisplay::drawText()" << std::endl;
    int fontHeight = fl_height(textFont, textSize);

    int Y = textArea.y + fontHeight;
    int baseline = Y - fl_descent();
    fl_color(textColor);
    fl_font(textFont, textSize);

    auto firstLineIter = firstLine < lines.size() ? lines.begin() + firstLine : lines.end();
    auto lastLineIter = firstLineIter + std::min(20, (int)(lines.end() - firstLineIter));
    std::span linesSpan(firstLineIter, lastLineIter);

    std::vector<Fl_Color> colors = {
        FL_RED,      FL_GREEN,      FL_BLUE,      FL_YELLOW,      FL_MAGENTA,      FL_CYAN,
        FL_DARK_RED, FL_DARK_GREEN, FL_DARK_BLUE, FL_DARK_YELLOW, FL_DARK_MAGENTA, FL_DARK_CYAN,
    };
    int colorIndex = 0;
    int offset = textArea.x;

    fl_push_clip(textArea.x, textArea.y, textArea.w, textArea.h);

    for (const auto& line : linesSpan)
    {
        const auto& startPos = line.first;
        const auto& endPos = line.second;
        const auto& lineLength = endPos - startPos;
        // Clear line
        Fl_Color bgcolor = color();
        fl_color(bgcolor);
        fl_rectf(offset, baseline - fontHeight + fl_descent(), textArea.w, fontHeight);

        // Draw selection background
        drawSelection(startPos, endPos, baseline);

        // Draw text
        fl_color(textColor);
        fl_draw(data + startPos, lineLength, textArea.x, baseline);
        baseline += fontHeight;
    }

    fl_pop_clip();
}

void LogDisplay::drawSelection(size_t startPos, size_t endPos, int baseline)
{
    // TODO: Duplicated from draw()
    int fontHeight = fl_height(textFont, textSize);

    auto selectionStart = selection.first;
    auto selectionEnd = selection.second;

    if (selectionStart > selectionEnd)
        std::swap(selectionStart, selectionEnd);

    // Selection begins in a line above the current one
    if (selectionStart < startPos && selectionEnd > startPos)
        selectionStart = startPos;

    const auto selectionLength = selectionEnd - selectionStart;

    if (selectionStart >= startPos && selectionStart <= endPos)
    {
        double selectionOffset = textArea.x + fl_width(data + startPos, selectionStart - startPos);
        double selectionWidth = selectionEnd > endPos ? textArea.w : fl_width(data + selectionStart, selectionLength);
        fl_color(FL_SELECTION_COLOR);
        fl_rectf(selectionOffset, baseline - fontHeight + fl_descent(), (int)selectionWidth, fontHeight);
    }
}

void LogDisplay::recalcSize()
{
    int X = x() + Fl::box_dx(box());
    int Y = y() + Fl::box_dy(box());
    int W = w() - Fl::box_dw(box());
    int H = h() - Fl::box_dh(box());

    const int lineNumbWidth = 0; // TODO: remove when line numbering is introduced
    const int scrollsize = Fl::scrollbar_size();

    textArea.x = X + LEFT_MARGIN + lineNumbWidth;
    textArea.y = Y + TOP_MARGIN;
    textArea.w = W - LEFT_MARGIN - RIGHT_MARGIN - lineNumbWidth - scrollsize;
    textArea.h = H - TOP_MARGIN - BOTTOM_MARGIN;

    // vScrollBar->resize(x() + w() - scrollsize, y(), scrollsize, h());
    vScrollBar->resize(X + W - scrollsize, textArea.y - TOP_MARGIN, scrollsize,
                       textArea.h + TOP_MARGIN + BOTTOM_MARGIN);
}
int LogDisplay::getRowByCharPos(unsigned long long charPos)
{
    for (int i = 0; i < lines.size(); i++)
    {
        if (charPos >= lines[i].first && charPos <= lines[i].second)
        {
            return i;
        }
    }
    return -1;
}
void LogDisplay::setSelectionStart(int event_x, int event_y)
{
    int row = getRowByMousePos(event_y);
    int column = getColumnByMousePos(row, event_x);
    int selectionStartIndex = column;
    selection.first = selectionStartIndex;
    selection.second = selectionStartIndex;
}

void LogDisplay::setSelectionEnd(int mouseX, int mouseY)
{
    int row = getRowByMousePos(mouseY);
    std::cout << "row: " << row << std::endl;

    int column = getColumnByMousePos(row, mouseX);
    std::cout << "column: " << (column - (int)lines[row].first) << std::endl;
    int selectionEndIndex = column;
    std::cout << "selectionEndIndex: " << selectionEndIndex << std::endl;
    selection.second = selectionEndIndex;
}
void LogDisplay::selectWord(int mouseX, int mouseY)
{
    int row = getRowByMousePos(mouseY);
    int column = getColumnByMousePos(row, mouseX);

    int selectionBegin = column;
    int selectionEnd = column;
    // Find word start
    while (selectionBegin > 0 && data[selectionBegin - 1] != ' ' && data[selectionBegin - 1] != '\n')
    {
        selectionBegin--;
    }
    // Find word end
    while (selectionEnd < dataSize && data[selectionEnd] != ' ' && data[selectionEnd] != '\n')
    {
        selectionEnd++;
    }
    selection.first = selectionBegin;
    selection.second = selectionEnd;
}
int LogDisplay::getRowByMousePos(int mouseY)
{
    int mousePos = mouseY - textArea.y; // relative to text area
    int rowBegin = 0;
    int rowEnd = 0;

    for (int i = 0; i < 100; i++) // TODO: This "100" should be calculated from LogDisplay height
    {
        rowBegin = rowEnd;
        rowEnd += fl_height(textFont, textSize);
        if (mousePos >= rowBegin && mousePos <= rowEnd)
        {
            return i + vScrollBar->value() - 1;
        }
    }
    return -1;
}
int LogDisplay::getColumnByMousePos(int row, int mouseX)
{
    assert(row >= 0 && row < lines.size());

    int mousePos = mouseX - textArea.x; // relative to text area
    auto line = lines[row];
    int lineBegin = line.first;
    int lineEnd = line.second;

    int columnBegin = lineBegin;
    int columnEnd = lineBegin;
    while (columnEnd <= lineEnd)
    {
        double textWidth = fl_width(data + columnBegin, columnEnd - columnBegin);
        if (mousePos >= textWidth)
        {
            // columnBegin = columnEnd;
            columnEnd++;
        }
        else
        {
            return columnEnd - 1;
        }
    }

    return lineEnd;
}

void LogDisplay::vScrollCallback(Fl_Scrollbar* w, LogDisplay* pThis)
{
    std::cout << "vScrollCallback: " << w->value() << std::endl;
    pThis->firstLine = w->value() - 1;
    // w->redraw();
    // pThis->redraw();
    pThis->damage(FL_DAMAGE_SCROLL);
}