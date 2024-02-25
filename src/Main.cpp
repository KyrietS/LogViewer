#include "Window.hpp"

#include <FL/Fl.H>
#include <cassert>
#include <fstream>
#include <string>

std::string readFile(const std::string& filename)
{
    std::ifstream file(filename);
    assert(file.is_open());
    const std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content;
}

int main()
{
    Fl::get_system_colors();
    const std::string content = readFile("pan-tadeusz.txt");

    Window window(600, 500);
    window.loadData(content.c_str(), content.size());
    window.show();

    return Fl::run();
}
