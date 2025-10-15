#pragma once

#include "RGB.hpp"
#include <Arduino.h>

#define LOG(FMT, ...) Serial.printf("[log] " FMT "\n", ##__VA_ARGS__)

#define PIN_LED0 21

inline void led0Write(const RGB& rgb) {
  rgbLedWriteOrdered(PIN_LED0, LED_COLOR_ORDER_RGB, rgb.r, rgb.g, rgb.b);
}
