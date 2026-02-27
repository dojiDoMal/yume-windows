
#include "engine_context.hpp"
#include "input/i_input_factory.hpp"

#include "scene.hpp"
#include "scene_manager.hpp"
#include "stb_image_header.hpp"

#include "game_object_manager.hpp"
#include "vector3.hpp"
#include "window/window_desc.hpp"
#include "window/window_manager.hpp"
#include "logger.hpp"
#include <SDL2/SDL_keycode.h>

#include <cmath>
#include <cstdio>
#include <memory>

#include <SDL2/SDL.h>

#ifdef PLATFORM_WEBGL
GraphicsAPI graphicsAPI = GraphicsAPI::WEBGL;
#else
// Escolha a API aqui: GraphicsAPI::OPENGL ou GraphicsAPI::VULKAN
GraphicsAPI graphicsAPI = GraphicsAPI::OPENGL;
#endif

Scene scene;
GameObjectManager gameObjects;
std::unique_ptr<WindowManager> screenManager;
std::unique_ptr<SceneManager> sceneManager;
RendererBackend* rendererBackend = nullptr;
WindowDesc winDesc;

auto inputMan = Yume::IInputFactory::create();
Yume::Context engine(inputMan);

void init() {
    
    winDesc.title = "Engine";
    winDesc.width = 800;
    winDesc.height = 600;
    
    screenManager = std::make_unique<WindowManager>();
    screenManager->setGraphicsApi(graphicsAPI);
    screenManager->init(winDesc);

    rendererBackend = screenManager->getRenderer()->getRendererBackend();

    sceneManager = std::make_unique<SceneManager>();
    sceneManager->setRendererBackend(*rendererBackend);
    //sceneManager->addScene("cena1", "scene_with_sprite.scnb");
    sceneManager->addScene("cena2", "scene.scnb");
    sceneManager->loadScene("cena2");

    engine.getInputSystem().bindKey(SDLK_ESCAPE, [&]() { 
        engine.getInputSystem().requestQuit(); 
    });

    engine.getInputSystem().bindKey(SDLK_SPACE, [&]() { 
        sceneManager->loadScene("cena2");
    });
}

#ifdef PLATFORM_WEBGL
#include <emscripten.h>
#endif

#ifdef PLATFORM_WEBGL
void main_loop() {

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT ||
            (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
            emscripten_cancel_main_loop();
            return;
        }
    }

    screenManager->getRenderer()->render(scene);
    SDL_GL_SwapWindow(screenManager->getWindow());
}
#else
void main_loop() {
    static Uint64 lastTime = SDL_GetPerformanceCounter();
    static float deltaTime = 0.0f;

    float moveSpeed = 2.0f;

    bool running = true;
    while (running) {
        Uint64 currentTime = SDL_GetPerformanceCounter();
        deltaTime = (float)(currentTime - lastTime) / SDL_GetPerformanceFrequency();
        lastTime = currentTime;

        engine.getInputSystem().processEvents();

        if (engine.getInputSystem().getQuitEvent()) {
            running = false;
        }

        Camera* cam = sceneManager->getActiveScene()->getCamera();
        if (cam) {
            Vector3 pos = cam->getPosition();
            Vector3 target = cam->getTarget();
            Vector3 delta = {0, 0, 0};

            float frameSpeed = moveSpeed * deltaTime;

            if (engine.getInputSystem().isKeyPressed(SDLK_w))
                delta.z -= frameSpeed;
            if (engine.getInputSystem().isKeyPressed(SDLK_s))
                delta.z += frameSpeed;
            if (engine.getInputSystem().isKeyPressed(SDLK_a))
                delta.x -= frameSpeed;
            if (engine.getInputSystem().isKeyPressed(SDLK_d))
                delta.x += frameSpeed;

            cam->setPosition({pos.x + delta.x, pos.y + delta.y, pos.z + delta.z});
            cam->setTarget({target.x + delta.x, target.y + delta.y, target.z + delta.z});
        }

        screenManager->render(*sceneManager->getActiveScene());
        screenManager->present();
    }

    SDL_Quit();
}

#endif

int main(int argc, char* argv[]) {
        Logger::init("engine");

        init();

#ifdef PLATFORM_WEBGL
        emscripten_set_main_loop(main_loop, 0, 1);
#else
        main_loop();
#endif

        Logger::shutdown();
        return 0;
}
