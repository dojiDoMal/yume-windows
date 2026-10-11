#define CLASS_NAME "Application"
#include "log_macros.hpp"

#include "application.hpp"

#include "color.hpp"
#include "components/script_services.hpp"
#include "components/text_renderer_component.hpp"
#include "logger.hpp"
#include "platform_paths.hpp"
#include "scene/world_object_manager.hpp"
#include "text_renderer.hpp"
#include "timer.hpp"
#include "window/mml/multimedia_layer_factory.hpp"

#ifndef __3DS__
#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>
#endif

#include <cstdio>

#ifdef PLATFORM_WEBGL
#include <emscripten.h>
#endif // PLATFORM_WEBGL

#ifdef __SWITCH__
#include <switch.h>
#endif // __SWITCH__

#ifdef __3DS__
#include <3ds.h>
#endif // __3DS__

namespace Yume {

// The engine Context (and the input/audio/display it exposes) is built in
// boot(), once the MultimediaLayer is up — it needs the resolved GraphicsAPI to
// pick a platform backend, which only happens after project.conf is loaded.
Application::Application() = default;

Application::~Application() = default;

bool Application::boot() {
#ifdef __3DS__
    Result rc = romfsInit();
    if (rc) {
        char rcBuf[32];
        std::snprintf(rcBuf, sizeof(rcBuf), "0x%08lX", (unsigned long)rc);
        LOG_ERROR(std::string("romfsInit failed: ") + rcBuf);
        return false;
    }
#elif defined(__SWITCH__)
    if (R_FAILED(romfsInit())) {
        LOG_ERROR("romfsInit failed");
        return false;
    }
#endif
    // Load project-level config (api/srgb/vsync + window + initial scene) from
    // project.conf. Missing/invalid file falls back to safe defaults inside the
    // loader. WebGL/Switch keep a fixed API below regardless of the file.
    // resolveAssetPath prefixes "romfs:/" on consoles and is a no-op elsewhere.
    rendererConfig = loadRendererConfig(resolveAssetPath(configPath));

    winDesc.title = rendererConfig.windowTitle;
    winDesc.width = rendererConfig.windowWidth;
    winDesc.height = rendererConfig.windowHeight;

    screenManager = std::make_unique<DisplayManager>();

    // Resolve the graphics API once: WebGL/Switch force their own regardless of
    // project.conf; the native desktop honors the file. The same API drives
    // both the DisplayManager (renderer backend) and the MultimediaLayer
    // factory (platform media backends), so compute it here and reuse it.
#if defined(PLATFORM_WEBGL)
    const GraphicsAPI graphicsApi = GraphicsAPI::WEBGL;
    screenManager->setGraphicsApi(graphicsApi);
#elif defined(__SWITCH__)
    // Switch uses the libnx OpenGL ES path (EGL/glad under the hood); it reports
    // GraphicsAPI::OPENGL like the desktop. project.conf's api is ignored, but
    // its srgb/vsync still apply to the OpenGL backend.
    const GraphicsAPI graphicsApi = GraphicsAPI::OPENGL;
    screenManager->setGraphicsApi(graphicsApi);
    screenManager->setRendererConfig(rendererConfig);
#elif defined(__3DS__)
    const GraphicsAPI graphicsApi = GraphicsAPI::CITRO3D;
    screenManager->setGraphicsApi(graphicsApi);
    screenManager->setRendererConfig(rendererConfig);
#else
    const GraphicsAPI graphicsApi = rendererConfig.api;
    screenManager->setGraphicsApi(graphicsApi);
    screenManager->setRendererConfig(rendererConfig);
#endif
    // The MultimediaLayer is the umbrella over display + input + audio and owns
    // the platform media lifecycle (e.g. SDL_Init/SDL_Quit). Bring it up before
    // the DisplayManager, which borrows its DisplayBackend to create the window.
    multimedia.reset(MultimediaLayerFactory::create(graphicsApi));
    if (!multimedia) {
        LOG_ERROR("Failed to create multimedia layer!");
        return false;
    }
    if (!multimedia->init()) {
        LOG_ERROR("Failed to initialize multimedia layer!");
        return false;
    }

    // Expose the shared subsystems (input/audio/display) through the Context now
    // that the multimedia layer is up.
    engine = std::make_unique<Context>(multimedia.get());

    if (!screenManager->init(multimedia->display(), winDesc)) {
        LOG_ERROR("DisplayManager init failed");
        return false;
    }

    rendererBackend = screenManager->getRenderer()->getRendererBackend();

    sceneManager = std::make_unique<SceneManager>();
    sceneManager->setRendererBackend(*rendererBackend);

    if (rendererConfig.scene.empty()) {
        LOG_WARN("No initial scene configured (project.conf 'scene' is empty)");
    } else {
        sceneManager->addScene("main", resolveAssetPath(rendererConfig.scene));
        sceneManager->loadScene("main");
    }

    // Expose engine services to scripts before any component starts: a
    // ScriptComponent's start() builds the Yume.InputSystem API from these.
    ScriptServices::configure(&engine->getInputSystem(), rendererConfig.inputAliases);

    // Component start() hooks run once the scene is fully built, before any
    // per-frame work and before onInit, so a ScriptComponent can read its
    // owner's Transform and bind its script's start()/update().
    startSceneComponents();

    // Project-specific setup runs after the engine is fully up and the scene is
    // loaded, so onInit can safely grab objects from the active scene.
    onInit();
    return true;
}

void Application::startSceneComponents() {
    Scene* scene = sceneManager ? sceneManager->getActiveScene() : nullptr;
    if (!scene)
        return;
    for (auto& obj : scene->getObjectManager()->getObjects()) {
        obj->startComponents();
    }
}

void Application::updateSceneComponents(float deltaTime) {
    Scene* scene = sceneManager ? sceneManager->getActiveScene() : nullptr;
    if (!scene)
        return;
    for (auto& obj : scene->getObjectManager()->getObjects()) {
        obj->updateComponents(deltaTime);
    }
}

void Application::mainLoop() {
    static Timer timer;

#if defined(__SWITCH__)
    while (appletMainLoop()) {
#elif defined(__3DS__)
    while (aptMainLoop()) {
#else
    bool running = true;
    while (running) {
#endif
        timer.tick();
        float deltaTime = timer.getDeltaTime();

        engine->getInputSystem().processEvents(deltaTime);
        if (engine->getInputSystem().getQuitEvent()) {
#if defined(__3DS__) || defined(__SWITCH__)
            break;
#else
            running = false;
#endif
        }

        // Component update() hooks (e.g. a ScriptComponent running a .ys)
        // run before the project's onUpdate, so project code can react to or
        // override whatever the scripts produced this frame.
        updateSceneComponents(deltaTime);

        // Project-specific per-frame logic (e.g. rotate the cube).
        onUpdate(deltaTime);

        screenManager->render(*sceneManager->getActiveScene());
        screenManager->present();
    }
}

void Application::shutdown() {
    // Tear down in the right order so SDL is finalized deterministically before
    // anything else (e.g. romfsExit on the Switch):
    //  1) Context is non-owning; drop it first so nothing reaches a dead layer.
    //  2) DisplayManager destroys the window through the MultimediaLayer's
    //     DisplayBackend, so it must go before the layer.
    //  3) MultimediaLayer runs SDL_Quit in its destructor (end()). Resetting it
    //     here makes that happen now rather than at Application destruction.
    // Do NOT call SDL_Quit() here: that would double-finalize SDL.
    engine.reset();
    screenManager.reset();
    multimedia.reset();
#if defined(__SWITCH__) || defined(__3DS__)
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
