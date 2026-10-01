// sandbox - the engine's built-in example / test app.
//
// Mirrors the Caminho 1 project shape (same as examples in yume-examples): the
// engine (yume_core) owns the window, renderer, scene loading and main loop via
// Yume::Application. This file only provides the project-specific bits by
// overriding the hooks:
//   - the initial scene + window + api come from project.conf
//   - onUpdate() spins the first mesh object in the scene every frame
//
// Build output and the compiled assets (scene.scnb, shaders, cube.obj,
// project.conf) land next to this executable (desktop) or inside the romfs
// (Switch), which the engine resolves at startup.

#include "application.hpp"
#include "scene.hpp"
#include "transform.hpp"
#include "vector3.hpp"
#include "world_object_manager.hpp"

// Force the dedicated GPU on laptops with switchable graphics. These exported
// symbols are read by the NVIDIA/AMD drivers at process start. Windows-only:
// __declspec is MSVC/MinGW syntax and must not reach the devkitPro GCC build.
#ifdef _WIN32
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

class SandboxApp : public Yume::Application {
  protected:
    void onUpdate(float deltaTime) override {
        // Spin the first object that has a mesh: 45 deg/s around X and Y.
        const float rotationSpeed = 45.0f; // degrees per second

        Scene* scene = scenes().getActiveScene();
        if (!scene)
            return;

        for (auto& obj : scene->getObjectManager()->getObjects()) {
            if (obj->hasMesh()) {
                Transform& transform = obj->getTransform();
                Vector3 rot = transform.getRotation();
                rot.x += rotationSpeed * deltaTime;
                rot.y += rotationSpeed * deltaTime;
                transform.setRotation(rot);
                break;
            }
        }
    }
};

int main() {
    SandboxApp app;
    return app.run();
}
