#include "logger.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

#ifdef __3DS__
#include <3ds.h>
#endif

static std::ofstream g_logFile;
static std::mutex g_logMutex;

static std::string formatDateTime(const char* format) {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto time = system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, format);
    return oss.str();
}

void Logger::init(const char* baseName) {
#if defined(__SWITCH__) || defined(__3DS__)
    // Switch: a saída vai por TRACE/nxlink (ver log_macros.hpp).
    // 3DS: a saída vai por svcOutputDebugString em log() — sem arquivo e sem
    // console na tela. Nada a abrir aqui.
    (void)baseName;
#else
    std::string filename =
        "logs/" + std::string(baseName) + "_" + formatDateTime("%Y-%m-%d_%H-%M-%S") + ".log";
    g_logFile.open(filename, std::ios::out | std::ios::app);
#endif
}

void Logger::shutdown() {
#if !defined(__SWITCH__) && !defined(__3DS__)
    if (g_logFile.is_open())
        g_logFile.close();
#endif
}

void Logger::log(const char* className, const char* methodName, const char* message) {
    std::lock_guard<std::mutex> lock(g_logMutex);

#ifdef __3DS__
    // Envia a linha ao log de debug do console. Em emulador (Citra/Azahar) isso
    // aparece na categoria Debug.Emulated; no hardware real é um no-op silencioso.
    // Não há consoleInit, então nada é desenhado em tela — as telas ficam livres
    // para o citro3d.
    std::ostringstream oss;
    oss << "[" << className << "::" << methodName << "] " << message << "\n";
    const std::string line = oss.str();
    svcOutputDebugString(line.c_str(), line.size());
#else
    if (!g_logFile.is_open())
        return;
    g_logFile << "[" << formatDateTime("%Y-%m-%d %H:%M:%S") << "] "
              << "[" << className << "::" << methodName << "] " << message << std::endl;
#endif
}
