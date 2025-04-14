#ifndef LOGGER_H
#define LOGGER_H

#include "./constants.h"
#include <cstdio>
#include <ctime>
#include <string>

class Logger {
public:
  enum LogLevel { DEBUG, INFO, WARNING, ERROR };

 void log_init() {
   setLoggingEnabled(is_logging);
    if (!isLoggingEnabled) {
      return;
    }
    time_t now = time(nullptr);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&now));
   std::string logfile = "logs/日志" + std::string(timestamp) + ".log";
   setLogFile(logfile.c_str());
  }

  static Logger &getInstance() {
    static Logger instance;
    return instance;
  }

  void setLogLevel(LogLevel level) { currentLevel = level; }

  void setLogFile(const char *filename) {
   if (logFile != nullptr) {
     fclose(logFile);
    }
   logFile = fopen(filename, "a");
  }

  // 修改 setLoggingEnabled 函数，接收一个布尔参数
  void setLoggingEnabled(bool enable) { isLoggingEnabled = enable; }

 void log(LogLevel level, const char *format, ...) {
    // 检查日志是否开启
    if (!isLoggingEnabled)
      return;

    if (level < currentLevel)
      return;

    time_t now = time(nullptr);
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));

    const char *levelStr;
    switch (level) {
    case DEBUG:
      levelStr = "DEBUG";
      break;
    case INFO:
      levelStr = "INFO";
      break;
    case WARNING:
      levelStr = "WARNING";
      break;
    case ERROR:
      levelStr = "ERROR";
      break;
    default:
      levelStr = "UNKNOWN";
      break;
    }

   if (logFile != nullptr) {
     fprintf(logFile, "[%s] [%s] ", timeStr, levelStr);
      va_list args;
      va_start(args, format);
     vfprintf(logFile, format, args);
      va_end(args);
     fprintf(logFile, "\n");
     fflush(logFile);
    }
  }

  ~Logger() {
   if (logFile != nullptr) {
     fclose(logFile);
    }
  }

private:
 Logger() : currentLevel(INFO), logFile(nullptr), isLoggingEnabled(true) {}
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  LogLevel currentLevel;
  FILE *logFile;
  // 添加一个布尔成员变量来控制日志是否开启
  bool isLoggingEnabled;
};

//// 修改 LOG_SET 宏，使其接收一个布尔参数
#define LOG_INIT() Logger::getInstance().log_init()
#define LOG_DEBUG(...) Logger::getInstance().log(Logger::DEBUG, __VA_ARGS__)
#define LOG_INFO(...) Logger::getInstance().log(Logger::INFO, __VA_ARGS__)
#define LOG_WARNING(...) Logger::getInstance().log(Logger::WARNING, __VA_ARGS__)
#define LOG_ERROR(...) Logger::getInstance().log(Logger::ERROR, __VA_ARGS__)

#endif // LOGGER_H
