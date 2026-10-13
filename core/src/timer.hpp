#pragma once

#ifdef __SWITCH__
#include <switch.h>
#elif defined(__3DS__)
#include <3ds.h>
#endif

/**
 * @brief Cronômetro de frames: mede o delta entre frames, FPS e tempo total.
 *
 * Chame tick() uma vez por frame; em seguida getDeltaTime() devolve o intervalo
 * desde o tick anterior. A implementação do relógio de alta resolução é
 * selecionada por plataforma (Windows, Switch, Emscripten ou std::chrono).
 *
 * @code
 * Timer timer;
 * while (running) {
 *     timer.tick();
 *     update(timer.getDeltaTime());
 * }
 * @endcode
 */
class Timer {
  private:
#ifdef _WIN32
    long long frequency;
    long long lastTime;
    long long startTime;
#elif defined(__SWITCH__)
    u64 lastTime;
    u64 startTime;
#elif defined(__3DS__)
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
    float deltaTime; ///< Duração do último frame, em segundos.

    /// @brief Teto do delta por frame (s). Absorve spikes de pausa do loop
    /// (arrastar a janela, suspensão do SO, breakpoint) para que nada integre
    /// um passo gigante. ~0,1s = 10 FPS: abaixo disso a cena só "engasga".
    static constexpr float MAX_DELTA_TIME = 0.1f;

  public:
    /** @brief Constrói o timer e marca o instante inicial. */
    Timer();

    /** @brief Atualiza o delta; chame uma vez por frame. */
    void tick();

    /** @brief Retorna o tempo do último frame, em segundos. */
    float getDeltaTime() const;

    /** @brief Retorna os frames por segundo estimados a partir do último delta. */
    float getFPS() const;

    /** @brief Retorna o tempo total decorrido desde a criação, em segundos. */
    float getTime() const;
};
