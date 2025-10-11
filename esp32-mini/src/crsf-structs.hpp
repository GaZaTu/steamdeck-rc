#pragma once

#include <cstdint>

namespace crsf {
constexpr auto BAUD = 400000;

constexpr auto CHANNEL_VALUE_MIN = 172;
constexpr auto CHANNEL_VALUE_MID = 991;
constexpr auto CHANNEL_VALUE_MAX = 1811;
constexpr auto CHANNEL_VALUE_RANGE = CHANNEL_VALUE_MAX - CHANNEL_VALUE_MIN;

// AETR
enum Channel {
  CHANNEL_AILERON,
  CHANNEL_ELEVATOR,
  CHANNEL_THROTTLE,
  CHANNEL_RUDDER,
  CHANNEL_AUX1, // (CH5)  ARM switch for Expresslrs
  CHANNEL_AUX2, // (CH6)  angel / airmode change
  CHANNEL_AUX3, // (CH7)  flip after crash
  CHANNEL_AUX4, // (CH8)
  CHANNEL_AUX5, // (CH9)
  CHANNEL_AUX6, // (CH10)
  CHANNEL_AUX7, // (CH11)
  CHANNEL_AUX8, // (CH12)
  CHANNEL_UNUSED1,
  CHANNEL_UNUSED2,
  CHANNEL_UNUSED3,
  CHANNEL_UNUSED4,
  CHANNEL_COUNT,
};

#define PACKED [[gnu::packed]]

constexpr auto FRAME_TIME_US = 4000;

enum FrameType {
  FRAMETYPE_GPS = 0x02,
  FRAMETYPE_VARIO = 0x07,
  FRAMETYPE_BATTERY_SENSOR = 0x08,
  FRAMETYPE_BARO_ALTITUDE = 0x09,
  FRAMETYPE_OPENTX_SYNC = 0x10,
  FRAMETYPE_LINK_STATISTICS = 0x14,
  FRAMETYPE_RC_CHANNELS_PACKED = 0x16,
  FRAMETYPE_ATTITUDE = 0x1E,
  FRAMETYPE_FLIGHT_MODE = 0x21,
  // Extended Header Frames, range: 0x28 to 0x96
  FRAMETYPE_DEVICE_PING = 0x28,
  FRAMETYPE_DEVICE_INFO = 0x29,
  FRAMETYPE_PARAMETER_SETTINGS_ENTRY = 0x2B,
  FRAMETYPE_PARAMETER_READ = 0x2C,
  FRAMETYPE_PARAMETER_WRITE = 0x2D,
  FRAMETYPE_ELRS_STATUS = 0x2E, // ELRS good/bad packet count and status flags
  FRAMETYPE_COMMAND = 0x32,
  FRAMETYPE_RADIO_ID = 0x3A,
  // Ardupilot frames
  FRAMETYPE_ARDUPILOT_PASSTHROUGH = 0x80,
  // MavLink frames
  FRAMETYPE_MAVLINK_ENVELOPE = 0xAA,
};

enum Address {
  ADDRESS_RC = 0xEA, // Remote Control
  ADDRESS_TX = 0xEE, // R/C Transmitter Module / Crossfire Tx
};

constexpr auto EXT_HEADER_BEGIN = 0x28;

struct PACKED Header {
  uint8_t sync;       // from crsf_addr_e
  uint8_t frame_size; // counts size after this byte, so it must be the payload size + 2 (type and crc)
  uint8_t frame_type; // from crsf_frame_type_e
};

#define CRSF_MK_FRAME(T)   \
  struct PACKED T##Frame { \
    Header h;              \
    T p;                   \
    uint8_t crc;           \
  }

// Used by extended header frames (type in range 0x28 to 0x96)
struct PACKED HeaderExt : public Header {
  // Extended fields
  uint8_t dest_addr;
  uint8_t orig_addr;
};

#define CRSF_MK_EXT_FRAME(T) \
  struct PACKED T##Frame {   \
    HeaderExt h;             \
    T p;                     \
    uint8_t crc;             \
  }

struct PACKED GPS {
  int32_t latitude;     // degree / 10`000`000
  int32_t longitude;    // degree / 10`000`000
  uint16_t groundspeed; // km/h / 100
  uint16_t heading;     // degree / 100
  uint16_t altitude;    // meter - 1000m offset
  uint8_t satellites;   // # of sats in view
};
CRSF_MK_FRAME(GPS);

struct PACKED Vario {
  int16_t v_speed; // Vertical speed cm/s
};
CRSF_MK_FRAME(Vario);

struct PACKED BatterySensor {
  int16_t voltage;             // Voltage (LSB = 10 µV)
  int16_t current;             // Current (LSB = 10 µA)
  uint32_t capacity_used : 24; // Capacity used (mAh)
  uint8_t remaining;           // Battery remaining (percent)
};
CRSF_MK_FRAME(BatterySensor);

struct PACKED BaroAltitude {
  uint16_t altitude_packed;     // Altitude above start (calibration) point
  int8_t vertical_speed_packed; // vertical speed. (see https://github.com/tbs-fpv/tbs-crsf-spec/blob/main/crsf.md#0x09-barometric-altitude--vertical-speed)
};
CRSF_MK_FRAME(BaroAltitude);

struct PACKED Attitude {
  int16_t pitch; // Pitch angle (LSB = 100 µrad)
  int16_t roll;  // Roll angle  (LSB = 100 µrad)
  int16_t yaw;   // Yaw angle   (LSB = 100 µrad)
};
CRSF_MK_FRAME(Attitude);

struct PACKED Telemetry {
  GPS gps;
  Vario vario;
  BatterySensor battery;
  BaroAltitude baroalt;
  Attitude attitude;
};

struct PACKED LinkStatistics {
  int8_t up_rssi_ant1;       // Uplink RSSI Antenna 1 (dBm * -1)
  int8_t up_rssi_ant2;       // Uplink RSSI Antenna 2 (dBm * -1)
  uint8_t up_link_quality;   // Uplink Package success rate / Link quality (%)
  int8_t up_snr;             // Uplink SNR (dB)
  uint8_t active_antenna;    // number of currently best antenna
  uint8_t rf_profile;        // enum {4fps = 0 , 50fps, 150fps}
  uint8_t up_rf_power;       // enum {0mW = 0, 10mW, 25mW, 100mW, 500mW, 1000mW, 2000mW, 250mW, 50mW}
  int8_t down_rssi;          // Downlink RSSI (dBm * -1)
  uint8_t down_link_quality; // Downlink Package success rate / Link quality (%)
  int8_t down_snr;           // Downlink SNR (dB)
};
CRSF_MK_FRAME(LinkStatistics);

struct PACKED ParameterHeader {
  uint8_t parameter;
  uint8_t chunks_remaining;
  uint8_t folder;
  uint8_t data_type;
  char name[];
};

union ParameterValue {
  struct PACKED {
    int32_t value;
    int32_t min_value;
    int32_t max_value;
    int32_t default_value;
    uint8_t decimal_point;
    int32_t step_size;
    char unit[1];
  } float_value;
  struct PACKED {
    uint8_t value;
    uint8_t min_value;
    uint8_t max_value;
    uint8_t default_value;
  } text_selection;
  struct PACKED {
    char value[1];
  } string_value;
};

struct PACKED ParameterWrite {
  uint8_t parameter;
  ParameterValue value;
};
// CRSF_MK_EXT_FRAME(ParameterWrite);

struct PACKED MavlinkEnvelope {
  uint8_t total_chunks : 4;  // total count of chunks
  uint8_t current_chunk : 4; // current chunk number
  uint8_t data_size;         // size of data (max 58)
  uint8_t data[];            // data array (58 bytes max)
};
// CRSF_MK_EXT_FRAME(MavlinkEnvelope);

#undef CRSF_MK_EXT_FRAME
#undef CRSF_MK_FRAME
#undef PACKED
} // namespace crsf

namespace elrs {
enum Parameter {
  PARAM_PACKET_RATE = 0x01,
  PARAM_TLM_RATIO = 0x02,
  PARAM_SWITCH_MODE = 0x03,
  PARAM_LINK_MODE = 0x04,
  PARAM_MODEL_MATCH = 0x05,
  PARAM_MAX_POWER = 0x07,
  PARAM_DYNAMIC = 0x08,
  PARAM_WIFI = 0xFE,
  PARAM_BIND = 0xFF,
};

enum PacketRate {
  PACKET_RATE_50HZ = 0x00,
  PACKET_RATE_100HZ,
  PACKET_RATE_150HZ,
  PACKET_RATE_250HZ,
  PACKET_RATE_333HZ,
  PACKET_RATE_500HZ,
  PACKET_RATE_D250HZ,
  PACKET_RATE_D500HZ,
  PACKET_RATE_F500HZ,
};

enum TLMRatio {
  TLM_RATIO_STD = 0x00,
  TLM_RATIO_OFF,
  TLM_RATIO_1_128,
  TLM_RATIO_1_64,
  TLM_RATIO_1_32,
  TLM_RATIO_1_16,
  TLM_RATIO_1_8,
  TLM_RATIO_1_4,
  TLM_RATIO_1_2,
};

enum SwitchMode {
  SWITCH_MODE_UNKNOWN = 0x00,
};

enum LinkMode {
  LINK_MODE_NORMAL = 0x00,
  LINK_MODE_MAVLINK,
};

enum ModelMatch {
  MODEL_MATCH_OFF = 0x00,
  MODEL_MATCH_ON,
};

enum Power {
  POWER_25MW = 0x00,
  POWER_50MW,
  POWER_100MW,
  POWER_250MW,
  POWER_500MW,
  POWER_1000MW,
};

enum Dynamic {
  DYNAMIC_UNKNOWN = 0x00,
};
} // namespace elrs
