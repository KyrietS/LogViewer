#include "LogDisplay.hpp"

#include "FL/Fl_Window.H"
#include "FL/fl_draw.H"

#include <iostream>
#include <span>
#include <array>

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
    //fl_rectf(x(), y(), w(), h(), bgcolor);
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
    if( damage() & FL_DAMAGE_SCROLL )
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

    switch (event)
    {
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

    std::vector<Fl_Color> colors = 
    {
        FL_RED,
		FL_GREEN,
		FL_BLUE,
		FL_YELLOW,
		FL_MAGENTA,
		FL_CYAN,
		FL_DARK_RED,
		FL_DARK_GREEN,
		FL_DARK_BLUE,
		FL_DARK_YELLOW,
		FL_DARK_MAGENTA,
		FL_DARK_CYAN,
	};
    int colorIndex = 0;
    int offset = textArea.x;

    fl_push_clip(textArea.x, textArea.y, textArea.w, textArea.h);

    for( const auto& line : linesSpan )
    {
        Fl_Color bgcolor = color();
        fl_color(bgcolor);
        fl_rectf(offset, baseline - fontHeight + fl_descent(), textArea.w, fontHeight);
        fl_color(textColor);
        fl_draw(data + line.first, line.second - line.first, textArea.x, baseline);
        baseline += fontHeight;
    }

    fl_pop_clip();

    return;

    for (const auto& line : linesSpan)
    {
        //Fl_Color bgcolor = color();
        //Fl_Color bgcolor = FL_MAGENTA;
        Fl_Color bgcolor = colors[colorIndex];
        colorIndex = (colorIndex + 1) % colors.size();
        fl_color(bgcolor);
        fl_rectf(offset, baseline - fontHeight, 5, fontHeight);
        offset += 5;
        fl_rectf(offset, baseline - fontHeight + fl_descent(), 5, fontHeight);
        //fl_rectf(textArea.x, baseline - fontHeight, 5, fontHeight);
        fl_color(textColor);
        fl_draw(data + line.first, line.second - line.first, textArea.x, baseline);
        baseline += fontHeight;
    }

    // fl_draw(data, dataSize, textArea.x, baseline);
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

void LogDisplay::vScrollCallback(Fl_Scrollbar* w, LogDisplay* pThis)
{
    std::cout << "vScrollCallback: " << w->value() << std::endl;
    pThis->firstLine = w->value() - 1;
    // w->redraw();
    //pThis->redraw();
    pThis->damage(FL_DAMAGE_SCROLL);
}