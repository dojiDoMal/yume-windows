#pragma once

#ifdef __SWITCH__
#include <switch.h>
#endif

class Timer {
  private:
#ifdef _WIN32
    long long frequency;
    long long lastTime;
    long long startTime;
#elif defined(__SWITCH__)
    u64 lastTime;
    u64 startTime;
#elif defined(__EMSCRIPTEN__)
    double lastTime;
    double startTime;
#else
    // std::chrono para outras plataformas
    long long lastTime;
    long long startTime;
#endif
    float deltaTime;

  public:
    Timer();
    void tick();
    float getDeltaTime() const;
    float getFPS() const;
    float getTime() const;
};
