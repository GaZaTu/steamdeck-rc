#pragma once

#include <cstdint>
#include <algorithm>

struct RGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;

  static RGB fromHSL(uint16_t h, float s, float l) {
    float a = s * std::min(l, 1.0f - l);
    auto f = [&](uint16_t n) {
      float k = (n + h / 30) % 12;
      return (uint8_t)((l - a * std::max(std::min(std::min(k - 3.0f, 9.0f - k), 1.0f), -1.0f)) * 255);
    };
    RGB rgb{f(0), f(8), f(4)};
    return rgb;
  }
};
