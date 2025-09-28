#pragma once

#include <Arduino.h>

#define PIN_BOARD_LED 21

#define LOG(FMT, ...) Serial.printf("[log]" FMT "\n", ##__VA_ARGS__)

struct RGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

void setBoardLED(const RGB& rgb) {
  rgbLedWrite(PIN_BOARD_LED, rgb.g, rgb.r, rgb.b);
}
