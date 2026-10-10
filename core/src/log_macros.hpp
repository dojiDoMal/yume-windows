/**
 * @file log_macros.hpp
 * @brief Macros de logging e integração de saída com nxlink no Switch.
 *
 * Forneça @c CLASS_NAME antes de incluir e use LOG_INFO/LOG_WARN/LOG_ERROR para
 * registrar mensagens: no desktop elas chamam Logger::log (incluindo classe e
 * método), e no Switch são redirecionadas para TRACE/printf. Quando
 * @c ENABLE_NXLINK está definido, a saída padrão é enviada a um servidor nxlink
 * para depuração remota.
 */
#ifndef LOGGER_MACROS_HPP
#define LOGGER_MACROS_HPP

#include "logger.hpp"
#include <string>

#ifndef ENABLE_NXLINK
#define TRACE(fmt, ...) ((void)0)
#else
#include <cstdio>
#include <switch.h>
#include <unistd.h>

/// @brief Imprime uma mensagem formatada com o nome da função (apenas no Switch/nxlink).
#define TRACE(fmt, ...) printf("%s: " fmt "\n", __PRETTY_FUNCTION__, ##__VA_ARGS__)

static int s_nxlinkSock = -1;

static void initNxLink() {
    if (R_FAILED(socketInitializeDefault()))
        return;

    s_nxlinkSock = nxlinkStdio();
    if (s_nxlinkSock >= 0)
        TRACE("printf output now goes to nxlink server");
    else
        socketExit();
}

static void deinitNxLink() {
    if (s_nxlinkSock >= 0) {
        close(s_nxlinkSock);
        socketExit();
        s_nxlinkSock = -1;
    }
}

extern "C" void userAppInit() { initNxLink(); }

extern "C" void userAppExit() { deinitNxLink(); }

#endif // ENABLE_NXLINK

#ifdef __SWITCH__

/// @brief Registra uma mensagem de informação.
#define LOG_INFO(msg) TRACE("[INFO] %s", (std::string(msg)).c_str())

/// @brief Registra uma mensagem de aviso.
#define LOG_WARN(msg) TRACE("[WARN] %s", (std::string(msg)).c_str())

/// @brief Registra uma mensagem de erro.
#define LOG_ERROR(msg) TRACE("[ERROR] %s", (std::string(msg)).c_str())

#else
// No 3DS usamos o mesmo ponto único (Logger::log) do desktop: lá a saída vai
// para svcOutputDebugString (log do emulador), sem desenhar na tela — por isso
// não há consoleInit em lugar nenhum e as duas telas ficam livres para o
// citro3d. As macros são idênticas às do desktop; a diferença de destino mora
// dentro de Logger::log (ver logger.cpp, ramo __3DS__).

#define LOG_INFO(msg) Logger::log(CLASS_NAME, __func__, (std::string("[INFO] ") + msg).c_str())

#define LOG_WARN(msg) Logger::log(CLASS_NAME, __func__, (std::string("[WARN] ") + msg).c_str())

#define LOG_ERROR(msg) Logger::log(CLASS_NAME, __func__, (std::string("[ERROR] ") + msg).c_str())

#endif // __SWITCH__

#endif // LOGGER_MACROS_HPP
