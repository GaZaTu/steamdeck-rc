#pragma once

#include <Arduino.h>
#include "RGB.hpp"

#define LOG(FMT, ...) Serial.printf("[log] " FMT "\n", ##__VA_ARGS__)

#define PIN_BOARD_LED 21

inline void setBoardLED(const RGB& rgb) {
  rgbLedWriteOrdered(PIN_BOARD_LED, LED_COLOR_ORDER_RGB, rgb.r, rgb.g, rgb.b);
}
