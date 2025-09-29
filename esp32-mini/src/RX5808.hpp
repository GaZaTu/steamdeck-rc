#pragma once

#include <Arduino.h>
#include "BasicTimer.hpp"

constexpr auto SPI_ADDRESS_SYNTH_A = 0x01;
constexpr auto SPI_ADDRESS_POWER = 0x0A;

class RX5808 {
private:
  int8_t _sck = -1;
  int8_t _data = -1;
  int8_t _ss = -1;
  int8_t _rssi = -1;

  void sendRegister(uint8_t address, uint32_t data) {
    sendSlaveSelect(LOW);

    sendBits(address, 4);
    sendBit(HIGH); // Enable write.

    sendBits(data, 20);

    // Finished clocking data in
    sendSlaveSelect(HIGH);
    digitalWrite(_sck, LOW);
    digitalWrite(_data, LOW);
  }

  void sendBits(uint32_t bits, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
      sendBit(bits & 0x1);
      bits >>= 1;
    }
  }

  void sendBit(uint8_t value) {
    digitalWrite(_sck, LOW);
    delayMicroseconds(1);

    digitalWrite(_data, value);
    delayMicroseconds(1);
    digitalWrite(_sck, HIGH);
    delayMicroseconds(1);

    digitalWrite(_sck, LOW);
    delayMicroseconds(1);
  }

  void sendSlaveSelect(uint8_t value) {
    digitalWrite(_ss, value);
    delayMicroseconds(1);
  }

  //
  // Sends SPI command to receiver module to change frequency.
  //
  // Format is LSB first, with the following bits in order:
  //     4 bits - address
  //     1 bit  - read/write enable
  //    20 bits - data
  //
  // Address for frequency select (Synth Register B) is 0x1
  // Expected data is (LSB):
  //     7 bits - A counter divider ratio
  //     1 bit  - seperator
  //    12 bits - N counter divder ratio
  //
  // Forumla for calculating N and A is:/
  //    F_lo = 2 * (N * 32 + A) * (F_osc / R)
  //    where:
  //        F_osc = 8 Mhz
  //        R = 8
  //
  // Refer to RTC6715 datasheet for further details.
  //
  void setSynthRegisterB(uint16_t value) {
    sendRegister(SPI_ADDRESS_SYNTH_A, value);
  }

  void setPowerDownRegister(uint32_t value) {
    sendRegister(SPI_ADDRESS_POWER, value);
  }

  void updateRssi() {
    analogRead(_rssi); // Fake read to let ADC settle.
    _rssi_raw = analogRead(_rssi);

    _rssi_value = constrain(map(_rssi_raw, _rssi_raw_min, _rssi_raw_max, 0, 100), 0, 100);

    if (_rssi_log_timer.hasTicked()) {
      for (uint8_t i = 0; i < sizeof(_rssi_last) - 1; i++) {
        _rssi_last[i] = _rssi_last[i + 1];
      }

      _rssi_last[sizeof(_rssi_last) - 1] = _rssi_value;

      _rssi_log_timer.reset();
    }
  }

  static uint16_t getSynthRegisterBByChannel(uint8_t index);

  uint8_t _active_channel = 0;

  uint8_t _rssi_value = 0;
  uint16_t _rssi_raw = 0;
  uint16_t _rssi_raw_min = 0;
  uint16_t _rssi_raw_max = 0;
  uint8_t _rssi_last[24] = {0};

  BasicTimer _rssi_stable_timer{35}; // 25?
  BasicTimer _rssi_log_timer{50};

public:
  void begin(int8_t sck = -1, int8_t data = -1, int8_t ss = -1, int8_t rssi = -1) {
    _sck = sck;
    _data = data;
    _ss = ss;
    _rssi = rssi;
  }

  void update() {
    if (_rssi_stable_timer.hasTicked()) {
      updateRssi();
    }
  }

  void disableAudio() {
    setPowerDownRegister(0b00010000110111110011);
  }

  void setChannel(uint8_t channel) {
    setSynthRegisterB(getSynthRegisterBByChannel(channel));
    _active_channel = channel;

    _rssi_stable_timer.reset();
  }

  uint8_t getChannel() const {
    return _active_channel;
  }

  uint8_t getRSSI() const {
    return _rssi_value;
  }

  static uint16_t getChannelFrequency(uint8_t index);

  static const char* getChannelName(uint8_t index);
};
