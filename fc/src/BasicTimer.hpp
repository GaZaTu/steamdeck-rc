#pragma once

#include <Arduino.h>

class BasicTimer {
private:
  uint32_t _nextTick;
  uint16_t _delay;
  bool _ticked;

public:
  BasicTimer(uint16_t delay) {
    _delay = delay;
    _nextTick = millis() + _delay;
    _ticked = false;
  }

  bool hasTicked() {
    if (_ticked) {
      return true;
    }

    if (millis() >= _nextTick) {
      _ticked = true;
      return true;
    }

    return false;
  }

  void reset() {
    _nextTick = millis() + _delay;
    _ticked = false;
  }

  bool resetIfTicked() {
    if (hasTicked()) {
      reset();
      return true;
    } else {
      return false;
    }
  }
};
