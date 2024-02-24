#pragma once

#include <FL/Fl_Box.H>
#include <FL/Fl_Flex.H>
#include <FL/fl_draw.H>
#include <cassert>

struct Separator : Fl_Widget
{
    Separator(const int x, const int y, int /* w */, const int h) : Fl_Widget(x, y, 2, h)
    {
    }

    void draw() override
    {
        fl_color(FL_DARK3);
        fl_yxline(x(), y(), y() + h());
        fl_color(FL_LIGHT3);
        fl_yxline(x() + 1, y(), y() + h());
    }
};

class StatusBar : public Fl_Flex
{
  public:
    StatusBar(const int x, const int y, const int w, const int h) : Fl_Flex(x, y, w, h, HORIZONTAL)
    {
        box(FL_FLAT_BOX);

        addMargin();
        addFixedLabel("my-project.laproj");
        addMargin(50);
        addSeparator();

        addFileStats();

        // Add a flexible space to push the labels to the right.
        new Fl_Box(0, 0, 0, 0, "");

        addSeparator();
        addFixedLabel("File loaded successfully");
        addMargin();

        Fl_Flex::end();
    }

    void setNumberOfLines(const size_t lines)
    {
        numberOfLines = lines;
        updateFileStats();
    }

  private:
    void addFileStats()
    {
        fileStats = addFixedLabel("");
        updateFileStats();
    }
    void updateFileStats()
    {
        assert(fileStats != nullptr);
        const std::string fileStatsText = "ln : 0/" + std::to_string(numberOfLines) + "    col : 0    pos : 0";
        setLabelText(fileStats, fileStatsText);
    }

    Fl_Box* addFixedLabel(const std::string& text)
    {
        auto* label = new Fl_Box(0, 0, 0, 0);
        setLabelText(label, text);
        return label;
    }

    void setLabelText(Fl_Box* label, const std::string& text)
    {
        label->copy_label(text.c_str());
        label->box(FL_FLAT_BOX);
        int width = 0;
        int height = 0;
        label->measure_label(width, height);
        fixed(label, width);
    }

    void addSeparator()
    {
        auto* separator = new Separator(0, 0, 0, 0);
        fixed(separator, 2);
        addMargin();
    }
    void addMargin(const int width = 10)
    {
        auto* margin = new Fl_Box(0, 0, 0, 0);
        margin->box(FL_FLAT_BOX);
        fixed(margin, width);
    }

    Fl_Box* fileStats = nullptr;
    size_t numberOfLines = 123;
};