#pragma once

#include <chrono>

class BasicTimer {
private:
  std::chrono::steady_clock _clock;

  std::chrono::milliseconds _delay;
  std::chrono::time_point<std::chrono::steady_clock> _next_tick;
  bool _ticked;

public:
  BasicTimer(std::chrono::milliseconds delay) {
    _delay = delay;
    _next_tick = _clock.now() + _delay;
    _ticked = false;
  }

  bool hasTicked() {
    if (_ticked) {
      return true;
    }

    if (_clock.now() >= _next_tick) {
      _ticked = true;
      return true;
    }

    return false;
  }

  void reset() {
    _next_tick = _clock.now() + _delay;
    _ticked = false;
  }

  bool resetIfTicked() {
    bool result = hasTicked();
    if (result) {
      reset();
    }
    return result;
  }
};
