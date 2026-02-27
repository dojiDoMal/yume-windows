#ifndef WINDOW_DESC_HPP
#define WINDOW_DESC_HPP

#include <string>

struct WindowDesc {
    std::string title;
    int width  = 800;
    int height = 600;
    unsigned int extraFlags = 0;
};

#endif