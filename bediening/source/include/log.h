#pragma once

// Minimal tagged serial logger. Compile-time level via -D APP_LOG_LEVEL=n (default INFO).
// Output format:  "   12345 I SW   message"

#include <Arduino.h>

#define APP_LOG_NONE 0
#define APP_LOG_ERROR 1
#define APP_LOG_WARN 2
#define APP_LOG_INFO 3
#define APP_LOG_DEBUG 4

#ifndef APP_LOG_LEVEL
#define APP_LOG_LEVEL APP_LOG_INFO
#endif

#define LOG_PRINT(ch, tag, fmt, ...) \
    Serial.printf("%8lu %c %-4s " fmt "\n", (unsigned long)millis(), ch, tag, ##__VA_ARGS__)

#define LOGE(tag, fmt, ...) do { if (APP_LOG_LEVEL >= APP_LOG_ERROR) LOG_PRINT('E', tag, fmt, ##__VA_ARGS__); } while (0)
#define LOGW(tag, fmt, ...) do { if (APP_LOG_LEVEL >= APP_LOG_WARN)  LOG_PRINT('W', tag, fmt, ##__VA_ARGS__); } while (0)
#define LOGI(tag, fmt, ...) do { if (APP_LOG_LEVEL >= APP_LOG_INFO)  LOG_PRINT('I', tag, fmt, ##__VA_ARGS__); } while (0)
#define LOGD(tag, fmt, ...) do { if (APP_LOG_LEVEL >= APP_LOG_DEBUG) LOG_PRINT('D', tag, fmt, ##__VA_ARGS__); } while (0)
