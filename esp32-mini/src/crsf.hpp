/**
 * Resources:
 *   - https://github.com/tbs-fpv/tbs-crsf-spec/blob/main/crsf.md
 *   - https://gist.github.com/GOROman/9c7eadf78eb522bbb801beb9162a8db5
 *   - https://github.com/kkbin505/Arduino-Transmitter-for-ELRS
 *   - https://github.com/danxdz/simpleTx_esp32
 *   - https://github.com/plutphil/SWUARTSerial/blob/main/README.md
 */

#pragma once

#include "crsf-structs.hpp"
#include <Arduino.h>
#include <deque>
#include <span>
#include <string>
#include <variant>

#ifndef NEW_SPAN
#ifdef __INTELLISENSE__
#define STD_SPAN_ARGS(PTR, LEN)
#else
#define STD_SPAN_ARGS(PTR, LEN) PTR, LEN
#endif
#endif

namespace crsf {
struct Parameter {
  uint8_t id;
  uint8_t data_type;
  std::string name;
  std::variant<double, uint8_t, std::string> value;
};

class Queue {
private:
  std::deque<std::vector<uint8_t>> _queue;

public:
  uint8_t _settings_chunk_idx[UINT8_MAX] = {0};

  size_t size() const noexcept {
    return _queue.size();
  }

  void push(std::vector<uint8_t>&& values) noexcept {
    _queue.emplace_back(std::move(values));
  }

  void push(std::span<const uint8_t> values) noexcept {
    std::vector<uint8_t> vector;
    vector.assign(values.begin(), values.end());
    push(std::move(vector));
  }

  void push_parameter_read(uint8_t parameter, uint8_t chunk_idx = 0) noexcept {
    _settings_chunk_idx[parameter] = chunk_idx;
    std::initializer_list<uint8_t> values{FRAMETYPE_PARAMETER_READ, ADDRESS_TX, ADDRESS_RC, parameter, _settings_chunk_idx[parameter]};
    if (chunk_idx > 0) {
      _queue.emplace_front(values);
    } else {
      _queue.emplace_back(values);
    }
  }

  void push_parameter_write(uint8_t parameter, uint8_t value) noexcept {
    push({FRAMETYPE_PARAMETER_WRITE, ADDRESS_TX, ADDRESS_RC, parameter, value});
  }

  std::span<const uint8_t> front() const noexcept {
    return _queue.front();
  }

  void pop_front() noexcept {
    _queue.pop_front();
  }
};

// class PhysicalLayer {
// public:
//   virtual void begin(std::string_view path) = 0;

//   virtual size_t available() = 0;

//   virtual size_t readBytes(std::span<uint8_t> buffer) = 0;

//   virtual size_t write(std::span<uint8_t> buffer) = 0;

//   virtual void flush(bool tx_only = false) = 0;
// };

class OneWire {
private:
  HardwareSerial& _serial;

  uint8_t _buffer[256];

  uint32_t _tx_timeframe = 0;

  bool _send_channels = true;

  Parameter _settings_entry;
  uint8_t _settings_buffer[256];
  uint16_t _settings_buffer_len = 0;

  uint8_t _mavlink_buffer[282];
  uint16_t _mavlink_buffer_len = 0;

  void writeChannels() noexcept;

  void write(std::span<const uint8_t> frame) noexcept;

  void write(const std::initializer_list<uint8_t>& frame) noexcept {
    write({STD_SPAN_ARGS(frame.begin(), frame.end())});
  }

  void ping() noexcept {
    write({FRAMETYPE_PARAMETER_WRITE, ADDRESS_TX, ADDRESS_RC, 0x00, 0x00});
  }

  void read(const Header& header, std::span<const uint8_t> payload) noexcept;

public:
  uint16_t channels[CHANNEL_COUNT];

  Queue tx_queue;

  LinkStatistics link_stats{0};

  Telemetry telemetry;

  std::function<void(const Parameter&)> on_settings_entry;

  std::function<void(std::span<const uint8_t>)> on_mavlink_telemetry;

  OneWire(HardwareSerial& serial) : _serial{serial} {
    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
      channels[i] = CHANNEL_VALUE_MIN;
    }
    channels[CHANNEL_THROTTLE] = CHANNEL_VALUE_MID;
  }

  void begin(int8_t rx_pin = -1, int8_t tx_pin = -1) noexcept;

  void tick() noexcept;
};
}; // namespace crsf
