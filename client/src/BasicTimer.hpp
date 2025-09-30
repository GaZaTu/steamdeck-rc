#pragma once

#include <chrono>

class BasicTimer {
private:
  std::chrono::steady_clock _clock;

  std::chrono::milliseconds _delay;
  std::chrono::time_point<std::chrono::steady_clock> _nextTick;
  bool _ticked;

public:
  BasicTimer(std::chrono::milliseconds delay) {
    _delay = delay;
    _nextTick = _clock.now() + _delay;
    _ticked = false;
  }

  const bool hasTicked() {
    if (_ticked) {
      return true;
    }

    if (_clock.now() >= _nextTick) {
      _ticked = true;
      return true;
    }

    return false;
  }

  void reset() {
    _nextTick = _clock.now() + _delay;
    _ticked = false;
  }
};
