/**
 * Resources:
 *   - https://github.com/tbs-fpv/tbs-crsf-spec/blob/main/crsf.md
 *   - https://gist.github.com/GOROman/9c7eadf78eb522bbb801beb9162a8db5
 *   - https://github.com/kkbin505/Arduino-Transmitter-for-ELRS
 *   - https://github.com/danxdz/simpleTx_esp32
 *   - https://github.com/plutphil/SWUARTSerial/blob/main/README.md
 *   - https://github.com/EdgeTX/edgetx/blob/main/radio/src/telemetry/crossfire.h
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

class Transmitter;

class TransmitQueue {
private:
  std::deque<std::vector<uint8_t>> _queue;

  uint8_t _settings_chunk_idx[UINT8_MAX] = {0};

public:
  friend Transmitter;

  inline size_t size() const noexcept {
    return _queue.size();
  }

  inline void push(std::vector<uint8_t>&& values) {
    _queue.emplace_back(std::move(values));
  }

  void push(std::span<const uint8_t> values) {
    std::vector<uint8_t> vector;
    vector.assign(values.begin(), values.end());
    push(std::move(vector));
  }

  void push_parameter_read(uint8_t parameter, uint8_t chunk_idx = 0) {
    _settings_chunk_idx[parameter] = chunk_idx;
    std::initializer_list<uint8_t> values{FRAMETYPE_PARAMETER_READ, ADDRESS_TX, ADDRESS_RC, parameter, _settings_chunk_idx[parameter]};
    if (chunk_idx > 0) {
      _queue.emplace_front(values);
    } else {
      _queue.emplace_back(values);
    }
  }

  inline void push_parameter_write(uint8_t parameter, uint8_t value) {
    push({FRAMETYPE_PARAMETER_WRITE, ADDRESS_TX, ADDRESS_RC, parameter, value});
  }

  inline std::span<const uint8_t> front() const noexcept {
    return _queue.front();
  }

  inline void pop_front() noexcept {
    _queue.pop_front();
  }
};

class Transmitter {
private:
  HardwareSerial& _serial;

  uint8_t _io_buffer[128];

  uint32_t _tx_timeframe = 0;

  bool _send_channels = true;

  Parameter _settings_entry;
  uint8_t _settings_buffer[256];
  uint16_t _settings_buffer_len = 0;

  uint8_t _mavlink_buffer[282];
  uint16_t _mavlink_buffer_len = 0;

  IRAM_ATTR void write(std::span<const uint8_t> frame);

  inline void write(const std::initializer_list<uint8_t>& frame) {
    write({STD_SPAN_ARGS(frame.begin(), frame.end())});
  }

  inline void writeChannels() {
    write(std::span<uint8_t>{STD_SPAN_ARGS((uint8_t*)&channels, sizeof(channels))});
  }

  void readRemoteFrame(std::span<const uint8_t> payload);

  void readSettingsEntry(std::span<const uint8_t> payload);

  void readMavlinkTelemetry(std::span<const uint8_t> payload);

  void read(const Header& header, std::span<const uint8_t> payload);

public:
  ChannelsPacked channels;

  TransmitQueue tx_queue;

  LinkStatistics link_stats{0};

  Telemetry telemetry;

  std::function<void(const Parameter&)> on_settings_entry;

  std::function<void(std::span<const uint8_t>)> on_mavlink_telemetry;

  explicit Transmitter(HardwareSerial& serial) : _serial{serial} {
    // nothing
  }

  void begin(int8_t rx_pin = -1, int8_t tx_pin = -1);

  IRAM_ATTR void tick();
};
}; // namespace crsf
