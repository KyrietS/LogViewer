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
    void drawSelection(size_t startPos, size_t endPos, int baseline);
    void recalcSize();

    EventStatus handleEvent(int event);
    EventStatus handleMousePressed();
    EventStatus handleMouseDragged();
    EventStatus handleMouseScroll(int event) const;
    EventStatus handleMouseMoved() const;

    void setCursor(Fl_Cursor cursorType) const;

    int getRowByCharPos(unsigned long long charPos);
    void setSelectionStart(int mouseX, int mouseY);
    void setSelectionEnd(int mouseX, int mouseY);
    void selectWord(int mouseX, int mouseY);
    int getRowByMousePos(int mouse_y);
    int getColumnByMousePos(int row, int mouse_x);

    static void vScrollCallback(Fl_Scrollbar* w, LogDisplay* pThis);

  private:
    struct
    {
        int x, y, w, h;
    } textArea;

    const char* data = nullptr;
    size_t dataSize = 0;

    size_t firstLine = 0; // The top visible line
    std::vector<std::pair<size_t, size_t>> lines;

    std::pair<size_t, size_t> selection = {0, 2}; // start, end

    Fl_Font textFont;
    Fl_Fontsize textSize;
    Fl_Color textColor;

    Fl_Scrollbar* vScrollBar;
};
