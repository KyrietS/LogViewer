#pragma once

#include <FL/Fl_Box.H>
#include <FL/Fl_Flex.H>
#include <FL/fl_draw.H>
#include <cassert>
#include <string>

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

class StatusBarWidget : public Fl_Flex
{
public:
    StatusBarWidget(const int x, const int y, const int w, const int h) : Fl_Flex(x, y, w, h, HORIZONTAL)
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
        statusInfo = addFixedLabel(" ");
        addMargin();

        Fl_Flex::end();
    }

    void setCursorPosition(const size_t lineNumber, const size_t columnNumber, const size_t globalOffset)
    {
        currentLine = lineNumber;
        currentColumn = columnNumber;
        currentPosition = globalOffset;
        updateFileStats();
    }

    void setNumberOfLines(const size_t lines)
    {
        numberOfLines = lines;
        updateFileStats();
    }

    void setStatusInformation( const std::string& text )
    {
        setLabelText(statusInfo, text);
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
        const std::string fileStatsText = "ln : " + std::to_string(currentLine) + "/" + std::to_string(numberOfLines) +
                                          "    col : " + std::to_string(currentColumn) +
                                          "    pos : " + std::to_string(currentPosition);
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
        damage(FL_DAMAGE_ALL);
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
    Fl_Box* statusInfo = nullptr;
    size_t numberOfLines = 0;
    size_t currentLine = 0;
    size_t currentColumn = 0;
    size_t currentPosition = 0;
};