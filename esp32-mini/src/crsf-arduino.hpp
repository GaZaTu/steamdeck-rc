#pragma once

#include "crsf.hpp"
#include <Arduino.h>

namespace crsf {
// class ArduinoPhysicalLayer : public PhysicalLayer {
// private:
//   HardwareSerial& _serial;

// public:
//   ArduinoPhysicalLayer(HardwareSerial& serial) : _serial{serial} {
//     // empty
//   }

//   void begin(int8_t rx_pin = -1, int8_t tx_pin = -1) {
//     pinMode(rx_pin, INPUT_PULLUP);
//     pinMode(tx_pin, OUTPUT);
//     _serial.begin(BAUD, SERIAL_8N1, rx_pin, tx_pin);
//   }

//   size_t available() override {
//     return _serial.available();
//   }

//   size_t readBytes(std::span<uint8_t> buffer) override {
//     return _serial.readBytes(buffer.data(), buffer.size());
//   }

//   size_t write(std::span<uint8_t> buffer) override {
//     return _serial.write(buffer.data(), buffer.size());
//   }

//   void flush(bool tx_only = false) override {
//     _serial.flush(tx_only);
//   }
// };
} // namespace crsf
