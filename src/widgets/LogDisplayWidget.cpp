#include "LogDisplayWidget.hpp"

#include "FL/Fl_Window.H"
#include "FL/fl_draw.H"

#include <array>
#include <cassert>
#include <cmath>
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

int getMouseX()
{
    return Fl::event_x();
}

int getMouseY()
{
    return Fl::event_y();
}

} // namespace

LogDisplayWidget::LogDisplayWidget(int X, int Y, int W, int H) : Fl_Group(X, Y, W, H)
{
    vScrollBar = new Fl_Scrollbar(0, 0, 1, 1);
    vScrollBar->callback(reinterpret_cast<Fl_Callback*>(vScrollCallback), this);
    vScrollBar->linesize(3);

    hScrollBar = new Fl_Scrollbar(0, 0, 1, 1);
    hScrollBar->type(FL_HORIZONTAL);
    hScrollBar->callback(reinterpret_cast<Fl_Callback*>(hScrollCallback), this);
    hScrollBar->linesize(50);

    textFont = FL_HELVETICA;
    textSize = FL_NORMAL_SIZE;
    textColor = FL_FOREGROUND_COLOR;

    recalcSize();

    color(FL_BACKGROUND2_COLOR, FL_SELECTION_COLOR);
    box(FL_DOWN_FRAME);
    end();
}

LogDisplayWidget::~LogDisplayWidget() = default;

void LogDisplayWidget::setData(const char* data, const size_t size)
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

    // In some places the line number is cast to int (for example when drawing the line number)
    // So for now I will allow for a file to have too many lines.
    assert(lines.size() < std::numeric_limits<int>::max() && "Too many lines!");

    const int numberOfLines = static_cast<int>(lines.size());
    vScrollBar->value(1, howManyLinesCanFit(), 1, numberOfLines);
}

const char* LogDisplayWidget::getData() const
{
    return data;
}

void LogDisplayWidget::draw()
{
    recalcSize();
    fl_push_clip(x(), y(), w(), h()); // prevent drawing outside widget area
    {
        // Draw to offscreen buffer to prevent flickering (double buffering)
        const Fl_Offscreen offscreenBuffer = fl_create_offscreen(x() + w(), y() + h());
        fl_begin_offscreen(offscreenBuffer);
        {
            drawBackground();
            drawText();

            vScrollBar->damage(FL_DAMAGE_ALL);
            hScrollBar->damage(FL_DAMAGE_ALL);
            update_child(*vScrollBar);
            update_child(*hScrollBar);
        }
        fl_end_offscreen();

        // Copy offscreen buffer to the screen (swap buffers)
        fl_copy_offscreen(x(), y(), w(), h(), offscreenBuffer, x(), y());

        // Clean up
        fl_delete_offscreen(offscreenBuffer);
    }
    fl_pop_clip();
}

void LogDisplayWidget::drawBackground() const
{
    fl_rectf(x(), y(), w(), h(), color());
    draw_box();

    // draw that little box in the corner of the scrollbars
    fl_rectf(hScrollBar->x() + hScrollBar->w(), hScrollBar->y(), vScrollBar->w(), hScrollBar->h(), FL_BACKGROUND_COLOR);
}

int LogDisplayWidget::handle(const int event)
{
    // I will pass the event to child widgets but I will not return early if they handled it.
    // No matter the result I will always continue to handle the event in this parent widget.
    const int childHandled = Fl_Group::handle(event);

    // The widget is not active
    if (!active_r() || !window())
    {
        return 0;
    }

    return static_cast<int>(handleEvent(event)) | childHandled;
}

LogDisplayWidget::EventStatus LogDisplayWidget::handleEvent(const int event)
{
    if (lines.empty() && event != FL_DND_ENTER && event != FL_DND_DRAG && event != FL_DND_RELEASE && event != FL_PASTE)
    {
        return EventStatus::NotHandled;
    }

    switch (event)
    {
    case FL_PUSH:
        return handleMousePressed();

    case FL_DRAG:
        return handleMouseDragged();

    case FL_MOUSEWHEEL:
        return handleMouseScrolled();

    case FL_ENTER:
    case FL_MOVE:
        return handleMouseMoved();

    case FL_LEAVE:
    case FL_HIDE:
        setCursor(FL_CURSOR_DEFAULT);
        return EventStatus::Handled;

    case FL_FOCUS:
        return EventStatus::Handled;

    case FL_UNFOCUS:
        setCursor(FL_CURSOR_DEFAULT);
        return EventStatus::Handled;

    case FL_KEYBOARD:
        return handleKeyboard();

    case FL_DND_ENTER:
    case FL_DND_DRAG:
    case FL_DND_RELEASE:
        return EventStatus::Handled;

    case FL_PASTE:
        // TODO: Load new file to the project
        std::cout << "Drag and dropped: " << Fl::event_text() << std::endl;
        return EventStatus::Handled;

    default:
        return EventStatus::NotHandled;
    }
}

void LogDisplayWidget::drawText()
{
    if (data == nullptr || lines.empty())
    {
        return;
    }

    fl_color(textColor);
    fl_font(textFont, textSize);

    const int lineHeight = getLineHeight();
    int baseline = textArea.y + lineHeight - fl_descent();

    const size_t topLineIndex = this->getIndexOfTopDisplayedLine();
    assert(topLineIndex < lines.size());
    const size_t howManyLinesToBeDrawn =
        std::min(static_cast<size_t>(howManyLinesCanFit() + 1), lines.size() - topLineIndex);
    const size_t bottomLineIndex = topLineIndex + howManyLinesToBeDrawn;

    // draw the background for line numbers on the left
    fl_rectf(lineNumbersArea.x, lineNumbersArea.y, lineNumbersArea.w, lineNumbersArea.h, lineNumbersBgColor);

    for (size_t lineIndex = topLineIndex; lineIndex < bottomLineIndex; ++lineIndex)
    {
        updateMaxLineWidth(lineIndex);

        const auto [startPos, endPos] = lines[lineIndex];
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
        const int lineNumber = static_cast<int>(lineIndex + 1);
        drawLineNumber(lineNumber, baseline, isCursorInThisLine ? bgcolor : lineNumbersBgColor);

        baseline += lineHeight;
    }
}

void LogDisplayWidget::drawSelection(const size_t startPos, const size_t endPos, const int baseline) const
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
        const int textAreaWithOffset = textArea.x - getHorizontalOffset();
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
void LogDisplayWidget::drawTextLine(const size_t lineBegin, const size_t lineEnd, const int baseline) const
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

void LogDisplayWidget::drawLineNumber(const int lineNumber, const int baseline, const Fl_Color bgcolor) const
{
    const int lineHeight = getLineHeight();
    fl_push_clip(lineNumbersArea.x, lineNumbersArea.y, lineNumbersArea.w, lineNumbersArea.h);

    fl_rectf(lineNumbersArea.x, baseline - lineHeight + fl_descent(), lineNumbersArea.w, lineHeight, bgcolor);

    const std::string lineNumberStr = std::to_string(lineNumber);
    fl_color(lineNumbersColor);
    fl_draw(lineNumberStr.c_str(), lineNumbersArea.x, baseline - lineHeight + fl_descent(),
            lineNumbersArea.w - RIGHT_MARGIN, lineHeight, FL_ALIGN_RIGHT);
    fl_pop_clip();
}

void LogDisplayWidget::recalcSize()
{
    const int X = x() + Fl::box_dx(box());
    const int Y = y() + Fl::box_dy(box());
    const int W = w() - Fl::box_dw(box());
    const int H = h() - Fl::box_dh(box());

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

int LogDisplayWidget::calcLineNumberWidth() const
{
    const size_t numOfLines = lines.size();
    std::string maxLineNumber = std::to_string(numOfLines);
    for (char& digit : maxLineNumber)
    {
        digit = '0'; // Probably the widest digit
    }

    fl_font(textFont, textSize);
    return static_cast<int>(std::ceil(fl_width(maxLineNumber.c_str(), static_cast<int>(maxLineNumber.size()))));
}

LogDisplayWidget::EventStatus LogDisplayWidget::handleMousePressed()
{
    if (Fl::event_inside(textArea.x, textArea.y, textArea.w, textArea.h))
    {
        handleMousePressedOnTextArea();
        return EventStatus::Handled;
    }

    if (Fl::event_inside(lineNumbersArea.x, lineNumbersArea.y, lineNumbersArea.w, lineNumbersArea.h))
    {
        selectLine(getMouseY());
        damage(FL_DAMAGE_SCROLL);
        return EventStatus::Handled;
    }

    return EventStatus::NotHandled;
}

void LogDisplayWidget::handleMousePressedOnTextArea()
{
    take_focus();
    cursorPos = getDataIndex(getMouseX(), getMouseY());

    // Selection with a shift key being held
    if (Fl::event_shift())
    {
        setSelectionEnd(getMouseX(), getMouseY());
        damage(FL_DAMAGE_SCROLL);
        return;
    }

    // Windows recognizes the third click as a normal click, so I need to hand-craft the triple click with a timer
    const bool windowsCompatTripleClick =
        Fl::seconds_since(lastDoubleClick) <= 0.5 && getMouseX() == doubleClickPos.x && getMouseY() == doubleClickPos.y;

    const bool doubleClick = Fl::event_clicks() == 1;
    const bool tripleClick = Fl::event_clicks() == 2 || windowsCompatTripleClick;

    if (tripleClick)
    {
        selectLine(getMouseY());
    }
    else if (doubleClick)
    {
        selectWord(getMouseX(), getMouseY());
        lastDoubleClick = Fl::now();
        doubleClickPos.x = getMouseX();
        doubleClickPos.y = getMouseY();
    }
    else
        setSelectionStart(getMouseX(), getMouseY());

    damage(FL_DAMAGE_SCROLL);
}

LogDisplayWidget::EventStatus LogDisplayWidget::handleMouseDragged()
{
    if (Fl::event_inside(lineNumbersArea.x, lineNumbersArea.y, lineNumbersArea.w, lineNumbersArea.h))
    {
        const size_t row = getLineIndex(getMouseY());
        selection.end = lines[row].second + 1;
    }
    else
    {
        setSelectionEnd(getMouseX(), getMouseY());
    }
    cursorPos = selection.end;
    damage(FL_DAMAGE_SCROLL);
    return EventStatus::Handled;
}

LogDisplayWidget::EventStatus LogDisplayWidget::handleMouseScrolled() const
{
    if (Fl::event_inside(this))
    {
        return EventStatus::Handled;
    }

    return EventStatus::NotHandled;
}
LogDisplayWidget::EventStatus LogDisplayWidget::handleMouseMoved() const
{
    if (Fl::event_inside(textArea.x, textArea.y, textArea.w, textArea.h))
    {
        setCursor(FL_CURSOR_INSERT);
        return EventStatus::Handled;
    }

    setCursor(FL_CURSOR_DEFAULT);
    return EventStatus::NotHandled;
}

LogDisplayWidget::EventStatus LogDisplayWidget::handleKeyboard()
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

std::string_view LogDisplayWidget::getSelectedText() const
{
    const size_t selectionStart = std::min(selection.begin, selection.end);
    const size_t selectionEnd = std::max(selection.begin, selection.end);
    return {data + selectionStart, selectionEnd - selectionStart};
}

void LogDisplayWidget::copySelectionToClipboard() const
{
    constexpr int clipboardDestination = 1; // 0 = selection buffer, 1 = clipboard, 2 = both
    const std::string_view selectedText = getSelectedText();
    Fl::copy(selectedText.data(), static_cast<int>(selectedText.length()), clipboardDestination);
}

void LogDisplayWidget::setCursor(const Fl_Cursor cursorType) const
{
    if (window())
    {
        window()->cursor(cursorType);
    }
}
int LogDisplayWidget::howManyLinesCanFit() const
{
    return textArea.h / getLineHeight();
}

size_t LogDisplayWidget::getIndexOfTopDisplayedLine() const
{
    const int index = vScrollBar->value() - 1;
    assert(index >= 0);
    return index;
}

int LogDisplayWidget::getHorizontalOffset() const
{
    return hScrollBar->value() - 1;
}

int LogDisplayWidget::getLineHeight() const
{
    return fl_height(textFont, textSize);
}

void LogDisplayWidget::findAndSetGlobalMaxLineWidth()
{
    double maxLineLength = 0;
    for (const auto& [lineBegin, lineEnd] : lines)
    {
        const size_t lineLength = lineEnd - lineBegin;
        double lineWidth = fl_width(data + lineBegin, static_cast<int>(lineLength));
        maxLineLength = std::max<double>(maxLineLength, lineWidth);
    }

    maxLineWidth = static_cast<int>(maxLineLength);
}

void LogDisplayWidget::updateMaxLineWidth(const size_t lineIndex)
{
    const auto [lineBegin, lineEnd] = lines[lineIndex];
    const size_t lineLength = lineEnd - lineBegin;
    const double lineWidth = fl_width(data + lineBegin, static_cast<int>(lineLength));

    if (lineWidth > maxLineWidth)
    {
        maxLineWidth = static_cast<int>(lineWidth);
        hScrollBar->value(hScrollBar->value(), textArea.w, 1, maxLineWidth);
    }
}

void LogDisplayWidget::setSelectionStart(const int mouseX, const int mouseY)
{
    const size_t selectionStartIndex = getDataIndex(mouseX, mouseY);
    selection.begin = selectionStartIndex;
    selection.end = selectionStartIndex;
}

void LogDisplayWidget::setSelectionEnd(const int mouseX, const int mouseY)
{
    const size_t selectionEndIndex = getDataIndex(mouseX, mouseY);
    selection.end = selectionEndIndex;
}

void LogDisplayWidget::selectWord(const int mouseX, const int mouseY)
{
    const size_t selectionEndIndex = getDataIndex(mouseX, mouseY);

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
void LogDisplayWidget::selectLine(const int mouseY)
{
    const size_t row = getLineIndex(mouseY);
    const size_t lineBegin = lines[row].first;
    const size_t lineEnd = lines[row].second + 1; // including newline character
    selection.begin = lineBegin;
    selection.end = lineEnd;
    cursorPos = selection.end;
}

size_t LogDisplayWidget::getDataIndex(const int mouseX, const int mouseY) const
{
    const size_t lineIndex = getLineIndex(mouseY);
    return getDataIndexInGivenLine(lineIndex, mouseX);
}

size_t LogDisplayWidget::getLineIndex(const int mouseY) const
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
            return i + getIndexOfTopDisplayedLine();
        }
    }
    return 0;
}

// Return the index of the character in a line pointed by the mouse.
size_t LogDisplayWidget::getDataIndexInGivenLine(const size_t lineIndex, const int mouseX) const
{
    if (lineIndex >= lines.size())
    {
        return dataSize;
    }
    if (mouseX < textArea.x)
    {
        return lines[lineIndex].first;
    }

    const int mousePos =
        mouseX - textArea.x + getHorizontalOffset(); // relative to text area including horizontal offset
    const auto [lineBegin, lineEnd] = lines[lineIndex];

    const size_t columnBegin = lineBegin;
    size_t columnEnd = lineBegin;
    while (columnEnd <= lineEnd)
    {
        const int textLength = static_cast<int>(columnEnd - columnBegin);
        const double textWidth = fl_width(data + columnBegin, textLength);
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

void LogDisplayWidget::vScrollCallback(Fl_Scrollbar*, LogDisplayWidget* pThis)
{
    pThis->damage(FL_DAMAGE_SCROLL);
}

void LogDisplayWidget::hScrollCallback(Fl_Scrollbar*, LogDisplayWidget* pThis)
{
    pThis->damage(FL_DAMAGE_SCROLL);
}
