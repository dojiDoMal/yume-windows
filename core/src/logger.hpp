#ifndef LOGGER_HPP
#define LOGGER_HPP

class Logger {
  public:
    static void init(const char* filename);
    static void shutdown();
    static void log(const char* className, const char* methodName, const char* message);
};

#endif
