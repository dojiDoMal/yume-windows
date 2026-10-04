#define CLASS_NAME "Application"
#include "log_macros.hpp"

#include "application.hpp"

#include "color.hpp"
#include "components/text_renderer_component.hpp"
#include "input/i_input_factory.hpp"
#include "logger.hpp"
#include "text_renderer.hpp"
#include "timer.hpp"
#include "scene/world_object_manager.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>
#include <cstdio>

#ifdef PLATFORM_WEBGL
#include <emscripten.h>
#endif

#ifdef __SWITCH__
#include <switch.h> // romfsInit / romfsExit
#include <unistd.h> // chdir
#endif

namespace Yume {

Application::Application() {
    // DesktopInput today; the factory abstracts the per-platform choice.
    inputMan.reset(Yume::IInputFactory::create());
    engine = std::make_unique<Context>(inputMan.get());
}

Application::~Application() = default;

bool Application::boot() {
#ifdef __SWITCH__
    // On the Switch there is no "folder next to the exe": runtime assets are
    // packed into the application's RomFS inside the .nro. Mount it and make it
    // the working directory, so every relative asset path the engine already
    // uses ("project.conf", "scene.scnb", "cube.obj", "flat.vxs.nxs", ...)
    // resolves against romfs:/ with no per-path changes.
    if (R_FAILED(romfsInit())) {
        LOG_ERROR("romfsInit failed");
        return false;
    }
    chdir("romfs:/");
#endif

    // The engine owns the SDL lifecycle. We build with SDL_MAIN_HANDLED (see
    // PC.cmake / Switch.cmake), so SDL does NOT hijack main() into SDL_main and
    // we don't link SDL2main. That means we must tell SDL the entry point is
    // ready and initialize the video subsystem ourselves before creating any
    // window.
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        LOG_ERROR(std::string("SDL_Init failed: ") + SDL_GetError());
        return false;
    }

    // Load project-level config (api/srgb/vsync + window + initial scene) from
    // project.conf. Missing/invalid file falls back to safe defaults inside the
    // loader. WebGL/Switch keep a fixed API below regardless of the file.
    rendererConfig = loadRendererConfig(configPath);

    winDesc.title = rendererConfig.windowTitle;
    winDesc.width = rendererConfig.windowWidth;
    winDesc.height = rendererConfig.windowHeight;

    screenManager = std::make_unique<WindowManager>();

#if defined(PLATFORM_WEBGL)
    // WebGL is fixed to its own API regardless of project.conf.
    screenManager->setGraphicsApi(GraphicsAPI::WEBGL);
#elif defined(__SWITCH__)
    // Switch uses the libnx OpenGL ES path (EGL/glad under the hood); it reports
    // GraphicsAPI::OPENGL like the desktop. project.conf's api is ignored, but
    // its srgb/vsync still apply to the OpenGL backend.
    screenManager->setGraphicsApi(GraphicsAPI::OPENGL);
    screenManager->setRendererConfig(rendererConfig);
#else
    screenManager->setGraphicsApi(rendererConfig.api);
    screenManager->setRendererConfig(rendererConfig);
#endif

    if (!screenManager->init(winDesc)) {
        LOG_ERROR("WindowManager init failed");
        return false;
    }

    rendererBackend = screenManager->getRenderer()->getRendererBackend();

    sceneManager = std::make_unique<SceneManager>();
    sceneManager->setRendererBackend(*rendererBackend);

    if (rendererConfig.scene.empty()) {
        LOG_WARN("No initial scene configured (project.conf 'scene' is empty)");
    } else {
        sceneManager->addScene("main", rendererConfig.scene);
        sceneManager->loadScene("main");
    }

    // Project-specific setup runs after the engine is fully up and the scene is
    // loaded, so onInit can safely grab objects from the active scene.
    onInit();
    return true;
}

void Application::updateDebugOverlay(float deltaTime) {
    // Scene totals, refreshed about once per second to avoid per-frame churn.
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

    // Toggle frustum culling with 'F' (edge-detected: one press = one toggle).
    {
        static bool prevFKey = false;
        bool fKey = engine->getInputSystem().isKeyPressed(SDLK_f);
        if (fKey && !prevFKey && rendererBackend) {
            rendererBackend->setFrustumCullingEnabled(!rendererBackend->isFrustumCullingEnabled());
        }
        prevFKey = fKey;
    }

    // Draw the overlay only if the scene provides a text renderer.
    TextRenderer* textRenderer = nullptr;
    for (auto& obj : sceneManager->getActiveScene()->getObjectManager()->getObjects()) {
        if (auto* trc = obj->getComponent<TextRendererComponent>()) {
            textRenderer = trc->getTextRenderer();
            break;
        }
    }

    if (!textRenderer)
        return;

    char buf[128];
    float tx = 20.0f, ty = 40.0f, lineH = 20.0f, scale = 20.0f;

    int drawnObjects = rendererBackend ? rendererBackend->getDrawnObjects() : displayObjects;
    int drawnTris = rendererBackend ? rendererBackend->getDrawnTris() : displayTris;
    int frustumCulled = rendererBackend ? rendererBackend->getFrustumCulledObjects() : 0;
    bool cullOn = rendererBackend ? rendererBackend->isFrustumCullingEnabled() : false;

    snprintf(buf, sizeof(buf), "FPS: %d", displayFPS);
    textRenderer->draw(buf, tx, ty, scale, COLOR::WHITE, winDesc.width, winDesc.height);
    snprintf(buf, sizeof(buf), "Objects: %d / %d (culled %d)", drawnObjects, displayObjects,
             frustumCulled);
    textRenderer->draw(buf, tx, ty + lineH, scale, COLOR::WHITE, winDesc.width, winDesc.height);
    snprintf(buf, sizeof(buf), "Vertices: %d", displayVerts);
    textRenderer->draw(buf, tx, ty + lineH * 2, scale, COLOR::WHITE, winDesc.width, winDesc.height);
    snprintf(buf, sizeof(buf), "Triangles: %d / %d", drawnTris, displayTris);
    textRenderer->draw(buf, tx, ty + lineH * 3, scale, COLOR::WHITE, winDesc.width, winDesc.height);
    snprintf(buf, sizeof(buf), "Frustum cull: %s (press F)", cullOn ? "ON" : "OFF");
    textRenderer->draw(buf, tx, ty + lineH * 4, scale, COLOR::WHITE, winDesc.width, winDesc.height);
}

void Application::mainLoop() {
    static Timer timer;

    bool running = true;
    while (running) {
        timer.tick();
        float deltaTime = timer.getDeltaTime();

        engine->getInputSystem().processEvents();
        if (engine->getInputSystem().getQuitEvent())
            running = false;

        // Project-specific per-frame logic (e.g. rotate the cube).
        onUpdate(deltaTime);

        // Engine debug overlay (FPS/stats + frustum-cull toggle). Only draws
        // when the scene has a TextRendererComponent, so it is harmless for
        // projects that don't ship one.
        updateDebugOverlay(deltaTime);

        screenManager->render(*sceneManager->getActiveScene());
        screenManager->present();
    }
}

void Application::shutdown() {
    SDL_Quit();
#ifdef __SWITCH__
    romfsExit();
#endif
}

int Application::run() {
    Logger::init("engine");

    if (!boot()) {
        Logger::shutdown();
        return 1;
    }

    mainLoop();

    shutdown();
    Logger::shutdown();
    return 0;
}

} // namespace Yume
