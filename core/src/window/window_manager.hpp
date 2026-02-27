#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include "../graphics_api.hpp"
#include "../renderer/renderer.hpp"
#include "window_desc.hpp"
#include <SDL2/SDL.h>

class WindowManager {
private:
    GraphicsAPI graphicsApi;
    SDL_Window* window = nullptr;
    Renderer* renderer = nullptr;

public:
    ~WindowManager();

    bool init(const WindowDesc& desc);
    void render(Scene& scene);
    SDL_Window* getWindow() {return window;};
    Renderer* getRenderer(){return renderer;}
    void present();
    void setGraphicsApi(const GraphicsAPI &api) { graphicsApi = api; }
};

#endif