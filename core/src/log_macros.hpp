#ifndef LOGGER_MACROS_HPP
#define LOGGER_MACROS_HPP

#include "logger.hpp"
#include <string>

#define LOG_INFO(msg) Logger::log(CLASS_NAME, __func__, (std::string("[INFO] ") + msg).c_str())

#define LOG_WARN(msg) Logger::log(CLASS_NAME, __func__, (std::string("[WARN] ") + msg).c_str())

#define LOG_ERROR(msg) Logger::log(CLASS_NAME, __func__, (std::string("[ERROR] ") + msg).c_str())

#endif
