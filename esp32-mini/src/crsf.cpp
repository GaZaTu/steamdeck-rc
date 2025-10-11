#include "crsf.hpp"

namespace crsf {
// clang-format off
static uint8_t crc8tab[] = {
  0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
  0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06, 0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
  0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
  0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2, 0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
  0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9, 0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
  0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
  0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
  0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F, 0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
  0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
  0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
  0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F, 0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
  0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
  0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
  0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74, 0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
  0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
  0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0, 0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9,
};
// clang-format on

static uint8_t crc8(const uint8_t* ptr, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    crc = crc8tab[crc ^ *ptr++];
  }
  return crc;
}

constexpr auto FRAMESIZE_RC_CHANNELS_PACKED = 24;

void OneWire::writeChannels() noexcept {
  _buffer[0] = ADDRESS_TX;
  _buffer[1] = FRAMESIZE_RC_CHANNELS_PACKED;
  _buffer[2] = FRAMETYPE_RC_CHANNELS_PACKED;
  _buffer[3] = (uint8_t)(channels[0] & 0x07FF);
  _buffer[4] = (uint8_t)((channels[0] & 0x07FF) >> 8 | (channels[1] & 0x07FF) << 3);
  _buffer[5] = (uint8_t)((channels[1] & 0x07FF) >> 5 | (channels[2] & 0x07FF) << 6);
  _buffer[6] = (uint8_t)((channels[2] & 0x07FF) >> 2);
  _buffer[7] = (uint8_t)((channels[2] & 0x07FF) >> 10 | (channels[3] & 0x07FF) << 1);
  _buffer[8] = (uint8_t)((channels[3] & 0x07FF) >> 7 | (channels[4] & 0x07FF) << 4);
  _buffer[9] = (uint8_t)((channels[4] & 0x07FF) >> 4 | (channels[5] & 0x07FF) << 7);
  _buffer[10] = (uint8_t)((channels[5] & 0x07FF) >> 1);
  _buffer[11] = (uint8_t)((channels[5] & 0x07FF) >> 9 | (channels[6] & 0x07FF) << 2);
  _buffer[12] = (uint8_t)((channels[6] & 0x07FF) >> 6 | (channels[7] & 0x07FF) << 5);
  _buffer[13] = (uint8_t)((channels[7] & 0x07FF) >> 3);
  _buffer[14] = (uint8_t)((channels[8] & 0x07FF));
  _buffer[15] = (uint8_t)((channels[8] & 0x07FF) >> 8 | (channels[9] & 0x07FF) << 3);
  _buffer[16] = (uint8_t)((channels[9] & 0x07FF) >> 5 | (channels[10] & 0x07FF) << 6);
  _buffer[17] = (uint8_t)((channels[10] & 0x07FF) >> 2);
  _buffer[18] = (uint8_t)((channels[10] & 0x07FF) >> 10 | (channels[11] & 0x07FF) << 1);
  _buffer[19] = (uint8_t)((channels[11] & 0x07FF) >> 7 | (channels[12] & 0x07FF) << 4);
  _buffer[20] = (uint8_t)((channels[12] & 0x07FF) >> 4 | (channels[13] & 0x07FF) << 7);
  _buffer[21] = (uint8_t)((channels[13] & 0x07FF) >> 1);
  _buffer[22] = (uint8_t)((channels[13] & 0x07FF) >> 9 | (channels[14] & 0x07FF) << 2);
  _buffer[23] = (uint8_t)((channels[14] & 0x07FF) >> 6 | (channels[15] & 0x07FF) << 5);
  _buffer[24] = (uint8_t)((channels[15] & 0x07FF) >> 3);
  _buffer[25] = crc8(&_buffer[2], _buffer[1] - 1);

  _serial.write(_buffer, FRAMESIZE_RC_CHANNELS_PACKED + 2);
  _serial.flush(true);
}

void OneWire::write(std::span<const uint8_t> frame) noexcept {
  _buffer[0] = ADDRESS_TX;
  _buffer[1] = frame.size() + 1;
  memcpy(&_buffer[2], frame.data(), frame.size());
  _buffer[2 + frame.size()] = crc8(&_buffer[2], _buffer[1] - 1);

  _serial.write(_buffer, frame.size() + 3);
  _serial.flush(true);
}

static bool isReadablePacket(const Header& header) {
  return header.sync != ADDRESS_TX && header.frame_type != FRAMETYPE_RC_CHANNELS_PACKED && header.frame_size != UINT8_MAX;
}

void OneWire::read(const Header& header, std::span<const uint8_t> payload) noexcept {
  // if (header->frame_type != 0x14 && header->frame_type != 0x3A) {
  //   if (is_ext) {
  //     Serial.printf("crsf: %02X %d %02X %02X %02X, len:%d crc0:%d crc1:%d\n", header->sync, header->frame_size, header->frame_type, header->dest_addr, header->orig_addr, payload.size(), crc_expected, crc_calculated);
  //   } else {
  //     Serial.printf("crsf: %02X %d %02X,       len:%d crc0:%d crc1:%d\n", header->sync, header->frame_size, header->frame_type, payload.size(), crc_expected, crc_calculated);
  //   }
  // }

  switch (header.frame_type) {
  case FRAMETYPE_GPS:
    memcpy(&telemetry.gps, payload.data(), payload.size());
    break;
  case FRAMETYPE_VARIO:
    memcpy(&telemetry.vario, payload.data(), payload.size());
    break;
  case FRAMETYPE_BATTERY_SENSOR:
    memcpy(&telemetry.battery, payload.data(), payload.size());
    break;
  case FRAMETYPE_BARO_ALTITUDE:
    memcpy(&telemetry.baroalt, payload.data(), payload.size());
    break;
  case FRAMETYPE_ATTITUDE:
    memcpy(&telemetry.attitude, payload.data(), payload.size());
    break;
  case FRAMETYPE_LINK_STATISTICS:
    memcpy(&link_stats, payload.data(), payload.size());
    break;

  case FRAMETYPE_RADIO_ID:
    break;

  case FRAMETYPE_PARAMETER_SETTINGS_ENTRY: {
    auto param_header = (ParameterHeader*)payload.data();
    if (_settings_buffer_len == 0) {
      _settings_entry.id = param_header->parameter;
      _settings_entry.data_type = param_header->data_type;
      _settings_entry.name = param_header->name;
      _settings_entry.value = 0.0;
    }
    auto param_header_len = (_settings_buffer_len == 0) ? (sizeof(ParameterHeader) + (strlen(param_header->name) + 1)) : 2;
    auto param_chunk_len = payload.size() - param_header_len;
    memcpy(&_settings_buffer[_settings_buffer_len], &payload[param_header_len], param_chunk_len);
    _settings_buffer_len += param_chunk_len;
    if (param_header->chunks_remaining == 0) {
      _settings_buffer_len = 0;
      auto param_value = (ParameterValue*)_settings_buffer;
      switch (_settings_entry.data_type) {
      case 0x08:
        _settings_entry.value = (double)param_value->float_value.value; // TODO: needs more work
        break;
      case 0x09:
        param_value = (ParameterValue*)&_settings_buffer[strlen((char*)_settings_buffer) + 1];
        _settings_entry.value = (uint8_t)param_value->text_selection.value;
        break;
      case 0x0A:
      case 0x0C:
        _settings_entry.value = (std::string)param_value->string_value.value;
        break;
      default:
        // TODO: log
        break;
      }
      if (on_settings_entry) {
        on_settings_entry(_settings_entry);
      }
    } else {
      tx_queue.push_parameter_read(_settings_entry.id, tx_queue._settings_chunk_idx[_settings_entry.id] + 1);
    }
  } break;

  case FRAMETYPE_MAVLINK_ENVELOPE: {
    memcpy(&_mavlink_buffer[_mavlink_buffer_len], &payload[3], payload[2]);
    _mavlink_buffer_len += payload[2];
    if (payload[1] == (payload[0] - 1)) {
      if (on_mavlink_telemetry) {
        on_mavlink_telemetry({STD_SPAN_ARGS(_mavlink_buffer, _mavlink_buffer_len)});
      }
      _mavlink_buffer_len = 0;
    }
  } break;

  default:
    // Serial.printf("    : %02X %02X\n", payload[0], payload[1]);
    break;
  }
}

void OneWire::begin(int8_t rx_pin, int8_t tx_pin) noexcept {
  pinMode(rx_pin, INPUT_PULLUP);
  pinMode(tx_pin, OUTPUT);
  _serial.begin(BAUD, SERIAL_8N1, rx_pin, tx_pin);

  _tx_timeframe = micros() + FRAME_TIME_US;
  ping();
}

void OneWire::tick() noexcept {
  auto time = micros();
  if (time > _tx_timeframe) {
    _tx_timeframe = time + FRAME_TIME_US;

    if (_send_channels || !tx_queue.size()) {
      _send_channels = false;

      writeChannels();
    } else {
      _send_channels = true;

      write(tx_queue.front());
      tx_queue.pop_front();
    }
  }

  if (_serial.available()) {
    auto size = _serial.readBytes(_buffer, _serial.available());

    auto idx = 0;
    while (idx < size) {
      auto header = (HeaderExt*)&_buffer[idx];
      auto header_size = (header->frame_type >= EXT_HEADER_BEGIN) ? sizeof(HeaderExt) : sizeof(Header);
      idx += header_size;

      std::span<uint8_t> payload{STD_SPAN_ARGS(&_buffer[idx], (size_t)(header->frame_size - (header_size - 1)))};
      idx += payload.size();

      auto crc_expected = _buffer[idx++];

      if (isReadablePacket(*header)) {
        // TODO: maybe skip crc check for performance reasons
        auto crc_calculated = crc8(&header->frame_type, header->frame_size - 1);
        if (crc_calculated != crc_expected) {
          // TODO: do something about this
        }

        read(*header, payload);
      }
    }
  }
}
}; // namespace crsf
