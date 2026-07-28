// Força uso da GPU dedicada em sistemas com múltiplas GPUs
// TODO: configuração de projeto
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

#include "components/text_renderer_component.hpp"
#include "engine_context.hpp"
#include "input/i_input_factory.hpp"

#include "scene.hpp"
#include "scene_manager.hpp"
#include "stb_image_header.hpp"

#include "logger.hpp"
#include "timer.hpp"
#include "vector3.hpp"
#include "window/window_desc.hpp"
#include "window/window_manager.hpp"
#include "world_object_manager.hpp"
#include <SDL2/SDL_keycode.h>

#include <cmath>
#include <cstdio>
#include <memory>

#include <SDL2/SDL.h>

#ifdef PLATFORM_WEBGL
GraphicsAPI graphicsAPI = GraphicsAPI::WEBGL;
#else
// ou GraphicsAPI::VULKAN
GraphicsAPI graphicsAPI = GraphicsAPI::OPENGL;
#endif

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
    // sceneManager->addScene("cena1", "scene_with_sprite.scnb");
    sceneManager->addScene("cena2", "scene.scnb");
    sceneManager->loadScene("cena2");

    engine.getInputSystem().bindKey(SDLK_ESCAPE, [&]() { engine.getInputSystem().requestQuit(); });

    engine.getInputSystem().bindKey(SDLK_SPACE, [&]() { sceneManager->loadScene("cena2"); });
}

#ifdef PLATFORM_WEBGL
#include <emscripten.h>

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
    static Timer timer;
    std::ostringstream ss;

    float moveSpeed = 2.0f;

    bool running = true;
    while (running) {
        timer.tick();
        float deltaTime = timer.getDeltaTime();

        // PRINT FPS + STATS
        static float elapsed = 0.0f;
        static int frameCount = 0;
        static int displayFPS = 0;
        static int displayObjects = 0, displayVerts = 0, displayTris = 0;

        elapsed += deltaTime;
        frameCount++;

        if (elapsed >= 1.0f) {
            displayFPS = frameCount;
            elapsed = 0.0f;
            frameCount = 0;

            displayObjects = 0;
            displayVerts = 0;
            displayTris = 0;
            for (auto& obj : sceneManager->getActiveScene()->getObjectManager()->getObjects()) {
                if (obj->hasMesh()) {
                    displayObjects++;
                    displayVerts += obj->getMesh()->getUniqueVertexCount();
                    displayTris += obj->getMesh()->getTriangleCount();
                }
            }
        }

        engine.getInputSystem().processEvents();

        if (engine.getInputSystem().getQuitEvent()) {
            running = false;
        }

        WorldObject* cameraObj = sceneManager->getActiveScene()->getCameraObject();
        if (cameraObj) {
            Transform& transform = cameraObj->getTransform();
            Vector3 pos = transform.getPosition();
            Vector3 rot = transform.getRotation();

            float frameSpeed = moveSpeed * deltaTime;

            // Calcular forward e right vectors da rotação (igual ao bindCamera)
            float yawRad = glm::radians(rot.y);
            float pitchRad = glm::radians(rot.x);

            Vector3 forward;
            forward.x = cos(pitchRad) * sin(yawRad);
            forward.y = sin(pitchRad);
            forward.z = cos(pitchRad) * cos(yawRad);

            // Normalizar
            float length =
                sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
            forward.x /= length;
            forward.y /= length;
            forward.z /= length;

            // Inverter (OpenGL usa -Z como forward)
            forward.x = -forward.x;
            forward.y = -forward.y;
            forward.z = -forward.z;

            Vector3 right = {cos(yawRad), 0.0f, -sin(yawRad)};

            // Movimento WASD
            Vector3 delta = {0, 0, 0};
            if (engine.getInputSystem().isKeyPressed(SDLK_w)) {
                delta.x += forward.x * frameSpeed;
                delta.z += forward.z * frameSpeed;
            }
            if (engine.getInputSystem().isKeyPressed(SDLK_s)) {
                delta.x -= forward.x * frameSpeed;
                delta.z -= forward.z * frameSpeed;
            }
            if (engine.getInputSystem().isKeyPressed(SDLK_a)) {
                delta.x -= right.x * frameSpeed;
                delta.z -= right.z * frameSpeed;
            }
            if (engine.getInputSystem().isKeyPressed(SDLK_d)) {
                delta.x += right.x * frameSpeed;
                delta.z += right.z * frameSpeed;
            }

            transform.setPosition({pos.x + delta.x, pos.y + delta.y, pos.z + delta.z});
            transform.setRotation(rot);
        }

        screenManager->render(*sceneManager->getActiveScene());

        // PRINT SCENE STATISTICS
        TextRenderer* textRenderer = nullptr;
        for (auto& obj : sceneManager->getActiveScene()->getObjectManager()->getObjects()) {
            if (auto* trc = obj->getComponent<TextRendererComponent>()) {
                textRenderer = trc->getTextRenderer();
                break;
            }
        }

        if (textRenderer) {
            char buf[64];
            float tx = 20.0f, ty = 40.0f, lineH = 20.0f, scale = 20.0f;

            snprintf(buf, sizeof(buf), "FPS: %d", displayFPS);
            textRenderer->draw(buf, tx, ty, scale, {1, 1, 1, 1}, winDesc.width, winDesc.height);
            snprintf(buf, sizeof(buf), "Objects: %d", displayObjects);
            textRenderer->draw(buf, tx, ty + lineH, scale, {1, 1, 1, 1}, winDesc.width,
                               winDesc.height);
            snprintf(buf, sizeof(buf), "Vertices: %d", displayVerts);
            textRenderer->draw(buf, tx, ty + lineH * 2, scale, {1, 1, 1, 1}, winDesc.width,
                               winDesc.height);
            snprintf(buf, sizeof(buf), "Faces: %d", displayTris);
            textRenderer->draw(buf, tx, ty + lineH * 3, scale, {1, 1, 1, 1}, winDesc.width,
                               winDesc.height);
            snprintf(buf, sizeof(buf), "Triangles: %d", displayTris);
            textRenderer->draw(buf, tx, ty + lineH * 4, scale, {1, 1, 1, 1}, winDesc.width,
                               winDesc.height);
        }
        // PRINT SCENE STATISTICS

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
