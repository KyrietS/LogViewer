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
    recalcSize();
    fl_push_clip(x(), y(), w(), h()); // prevent drawing outside widget area
    {
        if (damage() & FL_DAMAGE_ALL)
        {
            drawBackground();
            vScrollBar->damage(FL_DAMAGE_ALL);
        }
        if (damage() & FL_DAMAGE_SCROLL)
        {
            vScrollBar->damage(FL_DAMAGE_SCROLL);
        }
        update_child(*vScrollBar);

        drawText();
    }
    fl_pop_clip();
}
void LogDisplay::drawBackground() const
{
    fl_rectf(x(), y(), w(), h(), color());
    draw_box();
}

int LogDisplay::handle(const int event)
{
    if (!Fl::event_inside(textArea.x, textArea.y, textArea.w, textArea.h) && event != FL_LEAVE && event != FL_ENTER &&
        event != FL_MOVE && event != FL_FOCUS && event != FL_UNFOCUS && event != FL_KEYBOARD && event != FL_KEYUP)
    {
        return Fl_Group::handle(event);
    }

    // The widget is not active
    if (!active_r())
    {
        return 0;
    }

    return static_cast<int>(handleEvent(event));
}

LogDisplay::EventStatus LogDisplay::handleEvent(const int event)
{
    switch (event)
    {
    case FL_PUSH:
        return handleMousePressed();

    case FL_DRAG:
        return handleMouseDragged();

    case FL_MOUSEWHEEL:
        return handleMouseScroll(event);

    case FL_ENTER:
    case FL_MOVE:
        return handleMouseMoved();

    case FL_LEAVE:
    case FL_HIDE:
        setCursor(FL_CURSOR_DEFAULT);
        return EventStatus::Handled;

    default:
        return EventStatus::NotHandled;
    }
}

void LogDisplay::drawText()
{
    assert(firstLine < lines.size());

    fl_color(textColor);
    fl_font(textFont, textSize);

    const int fontHeight = fl_height(textFont, textSize);
    int baseline = textArea.y + fontHeight - fl_descent();

    const size_t howManyLinesCanFit = textArea.h / fontHeight;
    const auto howManyLinesToBeDrawn = std::min(howManyLinesCanFit, lines.size() - firstLine);

    using IterDiff = decltype(lines)::difference_type;
    const auto firstLineIter = lines.begin() + static_cast<IterDiff>(firstLine);
    const auto lastLineIter = firstLineIter + static_cast<IterDiff>(howManyLinesToBeDrawn);
    const std::span linesSpan(firstLineIter, lastLineIter);

    fl_push_clip(textArea.x, textArea.y, textArea.w, textArea.h);

    for (const auto& [startPos, endPos] : linesSpan)
    {
        const auto& lineLength = endPos - startPos;

        // Clear line
        Fl_Color bgcolor = color();
        fl_color(bgcolor);
        fl_rectf(textArea.x, baseline - fontHeight + fl_descent(), textArea.w, fontHeight);

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

    vScrollBar->resize(X + W - scrollsize, textArea.y - TOP_MARGIN, scrollsize,
                       textArea.h + TOP_MARGIN + BOTTOM_MARGIN);
}
LogDisplay::EventStatus LogDisplay::handleMousePressed()
{
    if (Fl::event_inside(textArea.x, textArea.y, textArea.w, textArea.h)) // TODO: Move to separate function
    {
        const bool doubleClick = Fl::event_clicks() != 0;
        if (doubleClick)
            selectWord(Fl::event_x(), Fl::event_y());
        else
            setSelectionStart(Fl::event_x(), Fl::event_y());
        damage(FL_DAMAGE_SCROLL);
        return EventStatus::Handled;
    }

    return EventStatus::NotHandled;
}
LogDisplay::EventStatus LogDisplay::handleMouseDragged()
{
    if (Fl::event_inside(textArea.x, textArea.y, textArea.w, textArea.h))
    {
        setSelectionEnd(Fl::event_x(), Fl::event_y());
        damage(FL_DAMAGE_SCROLL);
        return EventStatus::Handled;
    }

    return EventStatus::NotHandled;
}
LogDisplay::EventStatus LogDisplay::handleMouseScroll(const int event) const
{
    if (vScrollBar->handle(event))
    {
        return EventStatus::Handled;
    }

    return EventStatus::NotHandled;
}
LogDisplay::EventStatus LogDisplay::handleMouseMoved() const
{
    if (Fl::event_inside(textArea.x, textArea.y, textArea.w, textArea.h))
    {
        setCursor(FL_CURSOR_INSERT);
        return EventStatus::Handled;
    }

    setCursor(FL_CURSOR_DEFAULT);
    return EventStatus::NotHandled;
}
void LogDisplay::setCursor(const Fl_Cursor cursorType) const
{
    if (window())
    {
        window()->cursor(cursorType);
    }
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