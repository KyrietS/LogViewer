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

bool isWordSeparator(const char c)
{
    const std::string wordSeparator = " \n\t,.;:!?-()[]{}'\"/\\|<>+=*~`@#$%^&";
    return wordSeparator.find(c) != std::string::npos;
}

} // namespace

LogDisplay::LogDisplay(int X, int Y, int W, int H, const char* l) : Fl_Group(X, Y, W, H, l)
{
    vScrollBar = new Fl_Scrollbar(0, 0, 1, 1);
    vScrollBar->callback(reinterpret_cast<Fl_Callback*>(vScrollCallback), this);

    // TODO: Move to some recalc function
    vScrollBar->value(1, 1, 1, 1);
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
    if( data[ dataSize - 1 ] == '\n' )
    {
		lines.emplace_back(dataSize, dataSize);
	}

    const int numberOfLines = static_cast<int>(lines.size());
    vScrollBar->value(1, howManyLinesCanFit(), 1, numberOfLines);
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
            update_child(*vScrollBar);

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
    if (!Fl::event_inside(textArea.x, textArea.y, textArea.w, textArea.h) && event != FL_LEAVE && event != FL_ENTER &&
        event != FL_MOVE && event != FL_FOCUS && event != FL_UNFOCUS && event != FL_KEYBOARD && event != FL_KEYUP)
    {
        return Fl_Group::handle(event);
    }

    // The widget is not active
    if (!active_r() || !window())
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

    const auto howManyLinesToBeDrawn = std::min(static_cast<size_t>(howManyLinesCanFit() + 1), lines.size() - firstLine);

    using IterDiff = decltype(lines)::difference_type;
    const auto firstLineIter = lines.begin() + static_cast<IterDiff>(firstLine);
    const auto lastLineIter = firstLineIter + static_cast<IterDiff>(howManyLinesToBeDrawn);
    const std::span linesSpan(firstLineIter, lastLineIter);

    fl_push_clip(textArea.x, textArea.y, textArea.w, textArea.h);

    for (const auto& [startPos, endPos] : linesSpan)
    {
        const auto& lineLength = static_cast<int>(endPos - startPos);

        // Clear line
        const Fl_Color bgcolor = color();
        fl_color(bgcolor);
        fl_rectf(textArea.x, baseline - lineHeight + fl_descent(), textArea.w, lineHeight);

        // Draw selection background
        drawSelection(startPos, endPos, baseline);

        // Draw text
        fl_color(textColor);
        fl_draw(data + startPos, lineLength, textArea.x, baseline);
        baseline += lineHeight;
    }

    fl_pop_clip();
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
        const int lineHeight = getLineHeight();
        const double selectionOffset =
            textArea.x + fl_width(data + startPos, static_cast<int>(selectionStart - startPos));
        const double selectionWidth =
            selectionEnd > endPos ? textArea.w : fl_width(data + selectionStart, selectionLength);
        fl_color(FL_SELECTION_COLOR);
        fl_rectf(static_cast<int>(selectionOffset), baseline - lineHeight + fl_descent(),
                 static_cast<int>(selectionWidth), lineHeight);
    }
}

void LogDisplay::recalcSize()
{
    int X = x() + Fl::box_dx(box());
    int Y = y() + Fl::box_dy(box());
    int W = w() - Fl::box_dw(box());
    int H = h() - Fl::box_dh(box());

    constexpr int lineNumbWidth = 0; // TODO: remove when line numbering is introduced
    const int scrollsize = Fl::scrollbar_size();

    textArea.x = X + LEFT_MARGIN + lineNumbWidth;
    textArea.y = Y + TOP_MARGIN;
    textArea.w = W - LEFT_MARGIN - RIGHT_MARGIN - lineNumbWidth - scrollsize;
    textArea.h = H - TOP_MARGIN - BOTTOM_MARGIN;

    vScrollBar->resize(X + W - scrollsize, textArea.y - TOP_MARGIN, scrollsize,
                       textArea.h + TOP_MARGIN + BOTTOM_MARGIN);
    vScrollBar->value(vScrollBar->value(), howManyLinesCanFit(), 1, static_cast<int>(lines.size()));
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

LogDisplay::EventStatus LogDisplay::handleKeyboard()
{
    // Ctrl + C
    if( Fl::event_ctrl() && Fl::event_key() == 'c' )
    {
		copySelectionToClipboard();
		return EventStatus::Handled;
	}
    // Ctrl + A
    if( Fl::event_ctrl() && Fl::event_key() == 'a' )
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
    int clipboardDestination = 1; // 0 = selection buffer, 1 = clipboard, 2 = both
    Fl::copy(data + selection.begin, selection.end - selection.begin, clipboardDestination);
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

int LogDisplay::getLineHeight() const
{
    fl_font(textFont, textSize);
    return fl_height();
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
}

size_t LogDisplay::getCharIdxFromMousePos(const int mouseX, const int mouseY) const
{
    const int row = getRowByMousePos(mouseY);
    return getCharIdxFromRowAndMousePos(row, mouseX);
}

size_t LogDisplay::getRowByMousePos(const int mouseY) const
{
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
size_t LogDisplay::getCharIdxFromRowAndMousePos(const int row, const int mouseX) const
{
    if (row < 0)
    {
        return 0;
    }
    if( row >= lines.size() )
    {
        return dataSize;
	}

    const int mousePos = mouseX - textArea.x; // relative to text area
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