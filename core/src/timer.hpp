#pragma once

class Timer {
  private:
#ifdef _WIN32
    long long frequency;
    long long lastTime;
#elif defined(__EMSCRIPTEN__)
    double lastTime;
#else
    // std::chrono para outras plataformas
    long long lastTime;
#endif
    float deltaTime;

  public:
    Timer();
    void tick();
    float getDeltaTime() const;
    float getFPS() const;
};
