#pragma once
#include <FL/Fl_Flex.H>
#include <FL/fl_draw.H>
#include <functional>

class SearchBarWidget : public Fl_Flex
{
public:
    SearchBarWidget(int x, int y, int w, int h) : Fl_Flex(x, y, w, h, HORIZONTAL)
    {
        box(FL_FLAT_BOX);
        Fl_Color iconColor = fl_rgb_color(43, 43, 43);

        addMargin(4);

        auto searchIcon = new Fl_Button(0, 0, 20, 20, "@search");
        searchIcon->labelcolor(iconColor);
        searchIcon->box(FL_THIN_UP_BOX);
        int width;
        int height;
        searchIcon->measure_label(width, height);
        fixed(searchIcon, width + 8);
        searchIcon->clear_visible_focus();

        auto input = new Fl_Input(0, 0, 0, 0);
        fixed(input, 200);

        auto findPrevious = new Fl_Button(0, 0, 20, 20, "@8->");
        findPrevious->box(FL_THIN_UP_BOX);
        findPrevious->tooltip("Find previous");
        findPrevious->labelcolor(iconColor);

        fixed(findPrevious, h);
        auto findNext = new Fl_Button(0, 0, 20, 20, "@2->");
        findNext->box(FL_THIN_UP_BOX);
        findNext->tooltip("Find next");
        findNext->labelcolor(iconColor);
        fixed(findNext, h);

        // Add a flexible space to push the labels to the right.
        new Fl_Box(0, 0, 1, 0, "");

        auto closeIcon = new Fl_Button(0, 0, 20, 20, "\xe2\x9c\x95"); // The X mark: U+2715
        closeIcon->labelsize(15);
        closeIcon->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
        closeIcon->box(FL_FLAT_BOX);
        fixed(closeIcon, h);

        closeIcon->callback(reinterpret_cast<Fl_Callback*>(closeSearchBar), this);

        addMargin(4);

        Fl_Flex::end();
    }

    void onClose(std::function<void()> callback)
    {
        closeCallback = std::move(callback);
    }

private:
    void addMargin(const int width)
    {
        auto* margin = new Fl_Box(0, 0, 0, 0);
        margin->box(FL_FLAT_BOX);
        fixed(margin, width);
    }

    static void closeSearchBar(Fl_Widget* w, const SearchBarWidget* pThis)
    {
        if (pThis->closeCallback)
        {
            pThis->closeCallback();
        }
    }

    std::function<void()> closeCallback;
};
