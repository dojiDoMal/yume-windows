#ifndef YUME_APPLICATION_HPP
#define YUME_APPLICATION_HPP

#include "engine_context.hpp"
#include "input/i_input.hpp"
#include "renderer/renderer_backend.hpp"
#include "renderer_config.hpp"
#include "scene_manager.hpp"
#include "window/window_desc.hpp"
#include "window/window_manager.hpp"

#include <memory>
#include <string>

namespace Yume {

// Reusable application framework.
//
// The engine owns the boilerplate every project repeats: creating the window,
// the renderer backend, the scene manager, the input system and the main loop.
// A project subclasses Application and fills in only the parts specific to it:
//
//   - onInit():        called once after the engine is up and the initial scene
//                      (from project.conf) is loaded. Bind input, grab objects.
//   - onUpdate(dt):    called once per frame with the frame delta in seconds.
//                      Put per-frame game logic here (e.g. rotate an object).
//
// A project's main() just does:
//
//   class MyApp : public Yume::Application { ... };
//   int main() { MyApp app; return app.run(); }
//
// Config (window size/title, graphics api, initial scene) comes from
// project.conf, resolved relative to the working directory, which the build
// sets to the folder holding the compiled assets.
class Application {
  public:
    Application();
    virtual ~Application();

    // Boots the engine, runs the main loop until quit, then shuts down.
    // Returns a process exit code (0 = ok).
    int run();

  protected:
    // ---- Hooks for the project to override. Defaults are no-ops. ----
    virtual void onInit() {}
    virtual void onUpdate(float deltaTime) { (void)deltaTime; }

    // ---- Accessors for use inside onInit/onUpdate. ----
    SceneManager& scenes() { return *sceneManager; }
    IInput& input() { return engine->getInputSystem(); }
    RendererBackend* renderer() { return rendererBackend; }
    const WindowDesc& window() const { return winDesc; }

    // Project can tweak these before run() calls boot (e.g. in the ctor), but
    // project.conf overrides window title/size when present.
    WindowDesc winDesc;
    std::string configPath = "project.conf";

  private:
    bool boot();
    void mainLoop();
    void shutdown();

    // Engine-owned debug overlay: FPS/scene stats text + frustum-cull toggle.
    // Runs every frame independent of the project's onUpdate. No-op when the
    // active scene has no TextRendererComponent.
    void updateDebugOverlay(float deltaTime);

    RendererConfig rendererConfig;
    std::unique_ptr<WindowManager> screenManager;
    std::unique_ptr<SceneManager> sceneManager;
    RendererBackend* rendererBackend = nullptr;

    std::unique_ptr<IInput> inputMan;
    std::unique_ptr<Context> engine;
};

} // namespace Yume

#endif // YUME_APPLICATION_HPP
