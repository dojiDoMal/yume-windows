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

#define LOG_INFO(msg) TRACE("[INFO] %s", (std::string(msg)).c_str())

#define LOG_WARN(msg) TRACE("[WARN] %s", (std::string(msg)).c_str())

#define LOG_ERROR(msg) TRACE("[ERROR] %s", (std::string(msg)).c_str())

#else

#define LOG_INFO(msg) Logger::log(CLASS_NAME, __func__, (std::string("[INFO] ") + msg).c_str())

#define LOG_WARN(msg) Logger::log(CLASS_NAME, __func__, (std::string("[WARN] ") + msg).c_str())

#define LOG_ERROR(msg) Logger::log(CLASS_NAME, __func__, (std::string("[ERROR] ") + msg).c_str())

#endif // __SWITCH__

#endif // LOGGER_MACROS_HPP
