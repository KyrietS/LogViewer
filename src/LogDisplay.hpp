#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Scrollbar.H>
#include <vector>

class LogDisplay : public Fl_Group
{
    enum class EventStatus : int
    {
        NotHandled = 0,
        Handled = 1
    };

  public:
    LogDisplay(int X, int Y, int W, int H, const char* l = nullptr);
    ~LogDisplay() override;

    void setData(const char* data, size_t size);
    const char* getData() const;

  protected:
    void draw() override;
    int handle(int event) override;

  private:
    void drawBackground() const;
    void drawText();
    void drawSelection(size_t startPos, size_t endPos, int baseline) const;
    void drawTextLine(size_t lineBegin, size_t lineEnd, int baseline) const;
    void drawLineNumber(int lineNumber, int baseline, Fl_Color bgcolor) const;
    void recalcSize();
    int calcLineNumberWidth() const;

    EventStatus handleEvent(int event);
    EventStatus handleMousePressed();
    EventStatus handleMouseDragged();
    EventStatus handleMouseScrolled(int event) const;
    EventStatus handleMouseMoved() const;
    EventStatus handleKeyboard();

    void setCursor(Fl_Cursor cursorType) const;
    int howManyLinesCanFit() const;
    int getFirstLineIdx() const;
    int getHorizontalOffset() const;
    int getLineHeight() const;
    int getMaxLineWidth() const;

    void setSelectionStart(int mouseX, int mouseY);
    void setSelectionEnd(int mouseX, int mouseY);
    void selectWord(int mouseX, int mouseY);
    void selectLine(int mouseY);
    size_t getCharIdxFromMousePos(int mouseX, int mouseY) const;
    size_t getRowByMousePos(int mouseY) const;
    size_t getCharIdxFromRowAndMousePos(size_t row, int mouseX) const;

    void copySelectionToClipboard() const;

    static void vScrollCallback(Fl_Scrollbar* w, LogDisplay* pThis);
    static void hScrollCallback(Fl_Scrollbar* w, LogDisplay* pThis);

    // Text area within the widget
    struct
    {
        int x, y, w, h;
    } textArea{};

    // Line numbers displayed on the left size of text area
    struct
    {
        int x, y, w, h;
    } lineNumbersArea;
    Fl_Color lineNumbersColor = fl_rgb_color(150, 150, 150);
    Fl_Color lineNumbersBgColor = fl_rgb_color(245, 245, 245);

    // Text data
    const char* data = nullptr;
    size_t dataSize = 0;

    // Text selection
    struct
    {
        size_t begin, end;
    } selection{0, 0};

    // Cursor position
    // The cursor is not visible, but it is used internally
    size_t cursorPos = 0;

    // This needs to be cached because to get this value
    // we need to measure the width of each line of text.
    // TODO: CHANGE IT
    int maxLineWidth = 0;

    Fl_Timestamp lastDoubleClick = {};
    struct
    {
        int x, y;
    } doubleClickPos{};

    // An array of pairs of line start and end positions.
    // This helper array is created when the data is set.
    std::vector<std::pair<size_t, size_t>> lines;

    // Text properties
    Fl_Font textFont;
    Fl_Fontsize textSize;
    Fl_Color textColor;

    // Child widgets
    Fl_Scrollbar* vScrollBar;
    Fl_Scrollbar* hScrollBar;
};
