#include "LogDisplay.hpp"

#include "FL/Fl_Window.H"
#include "FL/fl_draw.H"

#include <array>
#include <cassert>
#include <iostream>
#include <span>
#include <string>

namespace
{
constexpr int TOP_MARGIN = 1;
constexpr int BOTTOM_MARGIN = 1;
constexpr int LEFT_MARGIN = 3;
constexpr int RIGHT_MARGIN = 3;

bool isWordSeparator(const char c)
{
    const std::string wordSeparator = " \n\t,.;:!?-()[]{}'\"/\\|<>+=*~`@#$%^&";
    return wordSeparator.find(c) != std::string::npos;
}

} // namespace

LogDisplay::LogDisplay(int X, int Y, int W, int H, const char* l) : Fl_Group(X, Y, W, H, l)
{
    // TODO: Memory leak
    vScrollBar = new Fl_Scrollbar(0, 0, 1, 1);
    vScrollBar->callback(reinterpret_cast<Fl_Callback*>(vScrollCallback), this);

    // TODO: Move to some recalc function
    vScrollBar->value(1, 1, 1, 1);
    vScrollBar->linesize(3);
    vScrollBar->set_visible();

    hScrollBar = new Fl_Scrollbar(0, 0, 1, 1);
    hScrollBar->type(FL_HORIZONTAL);
    hScrollBar->callback(reinterpret_cast<Fl_Callback*>(hScrollCallback), this);
    hScrollBar->value(1, 1, 1, 1);
    hScrollBar->linesize(50);
    hScrollBar->set_visible();

    textFont = FL_HELVETICA;
    textSize = FL_NORMAL_SIZE;
    textColor = FL_FOREGROUND_COLOR;

    recalcSize();

    color(FL_BACKGROUND2_COLOR, FL_SELECTION_COLOR);
    box(FL_DOWN_FRAME);
    end();
}

LogDisplay::~LogDisplay() = default;

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

    // Add an empty line if the last character is a newline
    if (data[dataSize - 1] == '\n')
    {
        lines.emplace_back(dataSize, dataSize);
    }

    const int numberOfLines = static_cast<int>(lines.size());
    vScrollBar->value(1, howManyLinesCanFit(), 1, numberOfLines);

    maxLineWidth = getMaxLineWidth(); // This is time consuming operation!
    hScrollBar->value(1, textArea.w, 1, maxLineWidth);
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
        // Draw to offscreen buffer to prevent flickering (double buffering)
        const Fl_Offscreen offscreenBuffer = fl_create_offscreen(w(), h());
        fl_begin_offscreen(offscreenBuffer);
        {
            drawBackground();

            vScrollBar->damage(FL_DAMAGE_ALL);
            hScrollBar->damage(FL_DAMAGE_ALL);
            update_child(*vScrollBar);
            update_child(*hScrollBar);

            drawText();
        }
        fl_end_offscreen();

        // Copy offscreen buffer to the screen (swap buffers)
        fl_copy_offscreen(x(), y(), w(), h(), offscreenBuffer, 0, 0);

        // Clean up
        fl_delete_offscreen(offscreenBuffer);
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
    // I will pass the event to child widgets but I will not return early if they handled it.
    // No matter the result I will always continue to handle the event in this parent widget.
    int childHandled = Fl_Group::handle(event);

    // The widget is not active
    if (!active_r() || !window())
    {
        return 0;
    }

    return static_cast<int>(handleEvent(event)) | childHandled;
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
        return handleMouseScrolled(event);

    case FL_ENTER:
    case FL_MOVE:
        return handleMouseMoved();

    case FL_LEAVE:
    case FL_HIDE:
        setCursor(FL_CURSOR_DEFAULT);
        return EventStatus::Handled;

    case FL_UNFOCUS:
        setCursor(FL_CURSOR_DEFAULT);
        return EventStatus::Handled;
    case FL_FOCUS:
        return EventStatus::Handled;

    case FL_KEYBOARD:
        return handleKeyboard();

    default:
        return EventStatus::NotHandled;
    }
}

void LogDisplay::drawText()
{
    if (data == nullptr)
    {
        return;
    }

    const int firstLine = this->getFirstLineIdx();
    assert(firstLine < lines.size());

    fl_color(textColor);
    fl_font(textFont, textSize);

    const int lineHeight = getLineHeight();
    int baseline = textArea.y + lineHeight - fl_descent();

    const auto howManyLinesToBeDrawn =
        std::min(static_cast<size_t>(howManyLinesCanFit() + 1), lines.size() - firstLine);

    using IterDiff = decltype(lines)::difference_type;
    const auto firstLineIter = lines.begin() + static_cast<IterDiff>(firstLine);
    const auto lastLineIter = firstLineIter + static_cast<IterDiff>(howManyLinesToBeDrawn);
    const std::span linesSpan(firstLineIter, lastLineIter);

    // draw the background for line numbers on the left
    fl_rectf(lineNumbersArea.x, lineNumbersArea.y, lineNumbersArea.w, lineNumbersArea.h, lineNumbersBgColor);

    // draw that little box in the corner of the scrollbars
    fl_rectf(hScrollBar->x() + hScrollBar->w(), hScrollBar->y(), vScrollBar->w(), hScrollBar->h(), FL_BACKGROUND_COLOR);

    int lineNumber = vScrollBar->value();
    for (const auto& [startPos, endPos] : linesSpan)
    {
        fl_push_clip(textArea.x, textArea.y, textArea.w, textArea.h);

        // Clear line
        const bool isCursorInThisLine = startPos <= cursorPos && cursorPos <= endPos;
        const Fl_Color bgcolor = isCursorInThisLine ? FL_DARK1 : color();
        fl_color(bgcolor);
        fl_rectf(textArea.x, baseline - lineHeight + fl_descent(), textArea.w, lineHeight);

        // Draw selection background
        drawSelection(startPos, endPos, baseline);

        // Draw text
        drawTextLine(startPos, endPos, baseline);
        fl_pop_clip();

        // Draw line number
        drawLineNumber(lineNumber, baseline, isCursorInThisLine ? bgcolor : lineNumbersBgColor);

        lineNumber += 1;
        baseline += lineHeight;
    }
}

void LogDisplay::drawSelection(const size_t startPos, const size_t endPos, int baseline) const
{
    auto selectionStart = selection.begin;
    auto selectionEnd = selection.end;

    if (selectionStart > selectionEnd)
        std::swap(selectionStart, selectionEnd);

    // Selection begins in a line above the current one
    if (selectionStart < startPos && selectionEnd > startPos)
        selectionStart = startPos;

    const auto selectionLength = static_cast<int>(selectionEnd - selectionStart);

    if (selectionStart >= startPos && selectionStart <= endPos)
    {
        // TODO: Create TextArea class and add this logic to a method.
        int textAreaWithOffset = textArea.x - getHorizontalOffset();
        const int lineHeight = getLineHeight();
        const double selectionOffset =
            textAreaWithOffset + fl_width(data + startPos, static_cast<int>(selectionStart - startPos));
        const double selectionWidth = selectionEnd > endPos ? std::max(textArea.w, maxLineWidth)
                                                            : fl_width(data + selectionStart, selectionLength);
        fl_color(selection_color());
        fl_rectf(static_cast<int>(selectionOffset), baseline - lineHeight + fl_descent(),
                 static_cast<int>(selectionWidth), lineHeight);
    }
}
void LogDisplay::drawTextLine(const size_t lineBegin, const size_t lineEnd, const int baseline) const
{
    const auto& lineLength = static_cast<int>(lineEnd - lineBegin);

    const size_t selectionBegin = std::min(selection.begin, selection.end);
    const size_t selectionEnd = std::max(selection.begin, selection.end);
    const int textPosition = textArea.x - getHorizontalOffset();

    // Selection is in a line above the current one
    if (selectionEnd < lineBegin)
    {
        fl_color(textColor);
        fl_draw(data + lineBegin, lineLength, textPosition, baseline);
        return;
    }

    // Selection is in a line below the current one
    if (selectionBegin > lineEnd)
    {
        fl_color(textColor);
        fl_draw(data + lineBegin, lineLength, textPosition, baseline);
        return;
    }

    // Selection overlaps the line
    const size_t selectedLineBegin = std::max(selectionBegin, lineBegin);
    const size_t selectedLineEnd = std::min(selectionEnd, lineEnd);

    const int beforeSelectionLength = static_cast<int>(selectedLineBegin - lineBegin);
    const int selectionLength = static_cast<int>(selectedLineEnd - selectedLineBegin);
    const int afterSelectionLength = static_cast<int>(lineEnd - selectedLineEnd);

    fl_color(textColor);
    fl_draw(data + lineBegin, beforeSelectionLength, textPosition, baseline);

    // Selected text will have white font color
    fl_color(FL_WHITE);
    const int selectionOffset = static_cast<int>(fl_width(data + lineBegin, beforeSelectionLength));
    fl_draw(data + selectedLineBegin, selectionLength, textPosition + selectionOffset, baseline);

    fl_color(textColor);
    const int afterSelectionOffset =
        static_cast<int>(fl_width(data + lineBegin, beforeSelectionLength + selectionLength));
    fl_draw(data + selectedLineEnd, afterSelectionLength, textPosition + afterSelectionOffset, baseline);
}

void LogDisplay::drawLineNumber(int lineNumber, int baseline, Fl_Color bgcolor) const
{
    int lineHeight = getLineHeight();
    fl_push_clip(lineNumbersArea.x, lineNumbersArea.y, lineNumbersArea.w, lineNumbersArea.h);

    fl_rectf(lineNumbersArea.x, baseline - lineHeight + fl_descent(), lineNumbersArea.w, lineHeight, bgcolor);

    std::string lineNumberStr = std::to_string(lineNumber);
    fl_color(lineNumbersColor);
    fl_draw(lineNumberStr.c_str(), lineNumbersArea.x, baseline - lineHeight + fl_descent(),
            lineNumbersArea.w - RIGHT_MARGIN, lineHeight, FL_ALIGN_RIGHT);
    fl_pop_clip();
}

void LogDisplay::recalcSize()
{
    int X = x() + Fl::box_dx(box());
    int Y = y() + Fl::box_dy(box());
    int W = w() - Fl::box_dw(box());
    int H = h() - Fl::box_dh(box());

    const int scrollsize = Fl::scrollbar_size();
    const int lineNumbersWidth = calcLineNumberWidth();

    lineNumbersArea.x = X;
    lineNumbersArea.y = Y;
    lineNumbersArea.w = lineNumbersWidth + LEFT_MARGIN + RIGHT_MARGIN;
    lineNumbersArea.h = H - scrollsize;

    textArea.x = X + lineNumbersArea.w + LEFT_MARGIN;
    textArea.y = Y + TOP_MARGIN;
    textArea.w = W - LEFT_MARGIN - RIGHT_MARGIN - lineNumbersArea.w - scrollsize;
    textArea.h = H - TOP_MARGIN - BOTTOM_MARGIN - scrollsize;

    vScrollBar->resize(X + W - scrollsize, textArea.y - TOP_MARGIN, scrollsize,
                       textArea.h + TOP_MARGIN + BOTTOM_MARGIN);
    hScrollBar->resize(X, Y + H - scrollsize, W - scrollsize, scrollsize);
    vScrollBar->value(vScrollBar->value(), howManyLinesCanFit(), 1, static_cast<int>(lines.size()));
    hScrollBar->value(hScrollBar->value(), textArea.w, 1, maxLineWidth);
}

int LogDisplay::calcLineNumberWidth() const
{
    size_t numOfLines = lines.size();
    std::string maxLineNumber = std::to_string(numOfLines);
    for (char& digit : maxLineNumber)
    {
        digit = '0'; // Probably the widest digit
    }

    fl_font(textFont, textSize);
    return static_cast<int>(std::ceil(fl_width(maxLineNumber.c_str(), maxLineNumber.size())));
}

LogDisplay::EventStatus LogDisplay::handleMousePressed()
{
    if (Fl::event_inside(textArea.x, textArea.y, textArea.w, textArea.h)) // TODO: Move to separate function
    {
        take_focus();
        cursorPos = getCharIdxFromMousePos(Fl::event_x(), Fl::event_y());

        // TODO: Move to separate function
        if (Fl::event_shift())
        {
            setSelectionEnd(Fl::event_x(), Fl::event_y());
            damage(FL_DAMAGE_SCROLL);
            return EventStatus::Handled;
        }

        // Windows recognizes the third click as a normal click, so I need to hand-craft the triple click with a timer
        const bool windowsCompatTripleClick = Fl::seconds_since(lastDoubleClick) <= 0.5 &&
                                              Fl::event_x() == doubleClickPos.x && Fl::event_y() == doubleClickPos.y;

        const bool doubleClick = Fl::event_clicks() == 1;
        const bool tripleClick = Fl::event_clicks() == 2 || windowsCompatTripleClick;

        if (tripleClick)
        {
            selectLine(Fl::event_y());
        }
        else if (doubleClick)
        {
            selectWord(Fl::event_x(), Fl::event_y());
            lastDoubleClick = Fl::now();
            doubleClickPos.x = Fl::event_x();
            doubleClickPos.y = Fl::event_y();
        }
        else
            setSelectionStart(Fl::event_x(), Fl::event_y());
        damage(FL_DAMAGE_SCROLL);
        return EventStatus::Handled;
    }

    if (Fl::event_inside(lineNumbersArea.x, lineNumbersArea.y, lineNumbersArea.w, lineNumbersArea.h))
    {
        selectLine(Fl::event_y());
        damage(FL_DAMAGE_SCROLL);
        return EventStatus::Handled;
    }

    return EventStatus::NotHandled;
}
LogDisplay::EventStatus LogDisplay::handleMouseDragged()
{
    if (Fl::event_inside(lineNumbersArea.x, lineNumbersArea.y, lineNumbersArea.w, lineNumbersArea.h))
    {
        // TODO: Move to separate function
        // Something like: extendSelectionTo(size_t)
        size_t row = getRowByMousePos(Fl::event_y());
        selection.end = lines[row].second + 1;
    }
    else
    {
        setSelectionEnd(Fl::event_x(), Fl::event_y());
    }
    cursorPos = selection.end;
    damage(FL_DAMAGE_SCROLL);
    return EventStatus::Handled;
}

LogDisplay::EventStatus LogDisplay::handleMouseScrolled(const int event) const
{
    if (Fl::event_inside(this))
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

LogDisplay::EventStatus LogDisplay::handleKeyboard()
{
    // Ctrl + C
    if (Fl::event_ctrl() && Fl::event_key() == 'c')
    {
        copySelectionToClipboard();
        return EventStatus::Handled;
    }
    // Ctrl + A
    if (Fl::event_ctrl() && Fl::event_key() == 'a')
    {
        selection.begin = 0;
        selection.end = dataSize;
        damage(FL_DAMAGE_SCROLL);
        return EventStatus::Handled;
    }

    return EventStatus::NotHandled;
}

void LogDisplay::copySelectionToClipboard() const
{
    constexpr int clipboardDestination = 1; // 0 = selection buffer, 1 = clipboard, 2 = both
    // TODO: Move getting selection text to a separate function
    const size_t selectionStart = std::min(selection.begin, selection.end);
    const size_t selectionEnd = std::max(selection.begin, selection.end);
    const int selectionLength = static_cast<int>(selectionEnd - selectionStart);
    Fl::copy(data + selectionStart, selectionLength, clipboardDestination);
}

void LogDisplay::setCursor(const Fl_Cursor cursorType) const
{
    if (window())
    {
        window()->cursor(cursorType);
    }
}
int LogDisplay::howManyLinesCanFit() const
{
    return textArea.h / getLineHeight();
}
int LogDisplay::getFirstLineIdx() const
{
    return vScrollBar->value() - 1;
}

int LogDisplay::getHorizontalOffset() const
{
    return hScrollBar->value() - 1;
}

int LogDisplay::getLineHeight() const
{
    return fl_height(textFont, textSize);
}

int LogDisplay::getMaxLineWidth() const
{
    // TODO: This is very stupid idea to measure the width of every line.
    // I should get the number of characters and multiply by the average
    // width of a character.
    // Or I can do something similar to Visual Studio Code, where it measures
    // the lines when are displayed and the horizontal scrollbar is updated
    // accordingly.
    double maxLineLength = 0;
    for (const auto& line : lines)
    {
        int lineLength = line.second - line.first;
        double lineWidth = fl_width(data + line.first, lineLength);
        maxLineLength = std::max<double>(maxLineLength, lineWidth);
    }

    return static_cast<int>(maxLineLength);
}

void LogDisplay::setSelectionStart(const int mouseX, const int mouseY)
{
    const size_t selectionStartIndex = getCharIdxFromMousePos(mouseX, mouseY);
    selection.begin = selectionStartIndex;
    selection.end = selectionStartIndex;
}

void LogDisplay::setSelectionEnd(const int mouseX, const int mouseY)
{
    const size_t selectionEndIndex = getCharIdxFromMousePos(mouseX, mouseY);
    selection.end = selectionEndIndex;
}

void LogDisplay::selectWord(const int mouseX, const int mouseY)
{
    const size_t selectionEndIndex = getCharIdxFromMousePos(mouseX, mouseY);

    size_t selectionBegin = selectionEndIndex;
    size_t selectionEnd = selectionEndIndex;
    // Find word start
    while (selectionBegin > 0 && !isWordSeparator(data[selectionBegin - 1]))
    {
        selectionBegin--;
    }
    // Find word end
    while (selectionEnd < dataSize && !isWordSeparator(data[selectionEnd]))
    {
        selectionEnd++;
    }
    selection.begin = selectionBegin;
    selection.end = selectionEnd;
    cursorPos = selection.end;
}
void LogDisplay::selectLine(int mouseY)
{
    const size_t row = getRowByMousePos(mouseY);
    const size_t lineBegin = lines[row].first;
    const size_t lineEnd = lines[row].second + 1; // including newline character
    selection.begin = lineBegin;
    selection.end = lineEnd;
    cursorPos = selection.end;
}

size_t LogDisplay::getCharIdxFromMousePos(const int mouseX, const int mouseY) const
{
    const size_t row = getRowByMousePos(mouseY);
    return getCharIdxFromRowAndMousePos(row, mouseX);
}

size_t LogDisplay::getRowByMousePos(const int mouseY) const
{
    if (mouseY < textArea.y)
    {
        return 0;
    }
    if (mouseY > textArea.y + textArea.h)
    {
        return lines.size() - 1;
    }

    const int mousePos = mouseY - textArea.y; // relative to text area
    const int howManyLines = howManyLinesCanFit() + 1;
    const int lineHeight = getLineHeight();
    int rowBegin = 0;
    int rowEnd = rowBegin;

    for (int i = 0; i < howManyLines; i++)
    {
        rowBegin = rowEnd;
        rowEnd += lineHeight;
        if (mousePos >= rowBegin && mousePos <= rowEnd)
        {
            return i + getFirstLineIdx();
        }
    }
    return 0;
}
// Return the index of the character pointed by the mouse.
size_t LogDisplay::getCharIdxFromRowAndMousePos(const size_t row, const int mouseX) const
{
    if (row >= lines.size())
    {
        return dataSize;
    }
    if (mouseX < textArea.x)
    {
        return lines[row].first;
    }

    const int mousePos =
        mouseX - textArea.x + getHorizontalOffset(); // relative to text area including horizontal offset
    const auto [lineBegin, lineEnd] = lines[row];

    const size_t columnBegin = lineBegin;
    size_t columnEnd = lineBegin;
    while (columnEnd <= lineEnd)
    {
        const int textSize = static_cast<int>(columnEnd - columnBegin);
        const double textWidth = fl_width(data + columnBegin, textSize);
        if (mousePos >= textWidth)
        {
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
    pThis->damage(FL_DAMAGE_SCROLL);
}

void LogDisplay::hScrollCallback(Fl_Scrollbar* w, LogDisplay* pThis)
{
    pThis->damage(FL_DAMAGE_SCROLL);
}
