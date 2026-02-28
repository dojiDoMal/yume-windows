#include "timer.hpp"

#ifdef _WIN32
#include <windows.h>
#elif defined(__EMSCRIPTEN__)
#include <emscripten.h>
#elif defined(__APPLE__)
#include <mach/mach_time.h>
#else
#include <chrono>
#endif

Timer::Timer() : deltaTime(0.0f) {
#ifdef _WIN32
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    frequency = freq.QuadPart;
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    lastTime = counter.QuadPart;
#elif defined(__EMSCRIPTEN__)
    lastTime = emscripten_get_now();
#else
    auto now = std::chrono::high_resolution_clock::now();
    lastTime =
        std::chrono::time_point_cast<std::chrono::nanoseconds>(now).time_since_epoch().count();
#endif
}

void Timer::tick() {
#ifdef _WIN32
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    long long currentTime = counter.QuadPart;
    deltaTime = (float)(currentTime - lastTime) / frequency;
    lastTime = currentTime;
#elif defined(__EMSCRIPTEN__)
    double currentTime = emscripten_get_now();
    deltaTime = (float)(currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;
#else
    auto now = std::chrono::high_resolution_clock::now();
    long long currentTime =
        std::chrono::time_point_cast<std::chrono::nanoseconds>(now).time_since_epoch().count();
    deltaTime = (float)(currentTime - lastTime) / 1000000000.0f;
    lastTime = currentTime;
#endif
}

float Timer::getDeltaTime() const { return deltaTime; }

float Timer::getFPS() const { return deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f; }