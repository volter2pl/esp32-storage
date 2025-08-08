#pragma once

#ifdef ESP32_STORAGE_DEBUG
#include <Arduino.h>
#define DBG(fmt, ...) Serial.printf("[esp32-storage] " fmt "\n", ##__VA_ARGS__)
#else
#define DBG(fmt, ...) do { } while (0)
#endif

