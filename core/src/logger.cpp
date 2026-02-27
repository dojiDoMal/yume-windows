#include "logger.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>


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
    std::string filename =
        "logs/" + std::string(baseName) + "_" + formatDateTime("%Y-%m-%d_%H-%M-%S") + ".log";
    g_logFile.open(filename, std::ios::out | std::ios::app);
}

void Logger::shutdown() {
    if (g_logFile.is_open())
        g_logFile.close();
}

void Logger::log(const char* className, const char* methodName, const char* message) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (!g_logFile.is_open())
        return;
    g_logFile << "[" << formatDateTime("%Y-%m-%d %H:%M:%S") << "] "
              << "[" << className << "::" << methodName << "] " << message << std::endl;
}
