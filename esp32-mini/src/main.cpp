#include "BasicTimer.hpp"
#include "crsf.hpp"
#include "esp32mini.hpp"
#include "rc-protocol.hpp"
#include <Arduino.h>

class RCGamepad {
public:
  static inline void write(const rc::RemoteEvent& remote_event) {
    Serial.write((uint8_t*)&remote_event, sizeof(remote_event));
  }

  static inline void read(rc::GamepadEvent& gamepad_event) {
    Serial.readBytes((uint8_t*)&gamepad_event, sizeof(gamepad_event));
  }

  static inline bool available() {
    return Serial.available();
  }
};

static crsf::OneWire crsf_serial{Serial0};

static rc::GamepadEvent gamepad_event;
static rc::RemoteEvent remote_event;

static int16_t axis_positions[rc::SDL_GAMEPAD_AXIS_COUNT] = {0};

static BasicTimer report_timer{500};

static bool armed = false;
static uint8_t arming = 0;

void setup() {
  Serial.begin(SERIAL_BAUD);

  setBoardLED({255, 0, 0});
  delay(1000);

  crsf_serial.begin(1, 2);
  crsf_serial.on_settings_entry = [](const crsf::Parameter& param) {
    if (param.value.index() == 1) {
      remote_event.type = rc::RemoteEvent::RC_EVENT_REPORT_PARAMETER;
      remote_event.report_parameter.parameter = param.id;
      remote_event.report_parameter.value = std::get<1>(param.value);
      RCGamepad::write(remote_event);
    }
  };

  setBoardLED({0, 255, 0});
}

void loop() {
  crsf_serial.tick();

  if (RCGamepad::available()) {
    RCGamepad::read(gamepad_event);
    switch (gamepad_event.type) {
    case rc::GamepadEvent::PAD_EVENT_GET_PARAMETER:
      switch (gamepad_event.get_parameter.parameter) {
      case rc::PARAM_PACKET_RATE:
      case rc::PARAM_TLM_RATIO:
      case rc::PARAM_SWITCH_MODE:
      case rc::PARAM_LINK_MODE:
      case rc::PARAM_MODEL_MATCH:
      case rc::PARAM_MAX_POWER:
      case rc::PARAM_WIFI:
        crsf_serial.tx_queue.push_parameter_read(gamepad_event.get_parameter.parameter);
        break;
      }
      break;

    case rc::GamepadEvent::PAD_EVENT_SET_PARAMETER:
      switch (gamepad_event.set_parameter.parameter) {
      case rc::PARAM_PACKET_RATE:
      case rc::PARAM_TLM_RATIO:
      case rc::PARAM_SWITCH_MODE:
      case rc::PARAM_LINK_MODE:
      case rc::PARAM_MODEL_MATCH:
      case rc::PARAM_MAX_POWER:
      case rc::PARAM_WIFI:
        crsf_serial.tx_queue.push_parameter_write(gamepad_event.set_parameter.parameter, (uint8_t)gamepad_event.set_parameter.value);
        break;
      }
      break;

    case rc::GamepadEvent::SDL_EVENT_GAMEPAD_BUTTON_DOWN:
      switch (gamepad_event.button_down.button) {
      case rc::SDL_GAMEPAD_BUTTON_LEFT_STICK:
      case rc::SDL_GAMEPAD_BUTTON_RIGHT_STICK:
      case rc::SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
      case rc::SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
        arming += gamepad_event.button_down.button;
        if (arming == (rc::SDL_GAMEPAD_BUTTON_LEFT_STICK + rc::SDL_GAMEPAD_BUTTON_RIGHT_STICK + rc::SDL_GAMEPAD_BUTTON_LEFT_SHOULDER + rc::SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)) {
          armed = !armed;
          crsf_serial.channels[crsf::CHANNEL_AUX1] = armed ? crsf::CHANNEL_VALUE_MAX : crsf::CHANNEL_VALUE_MIN;

          remote_event.type = rc::RemoteEvent::RC_EVENT_REPORT_ARMED;
          remote_event.report_armed.armed = armed;
          RCGamepad::write(remote_event);
        }
        break;
      }
      break;

    case rc::GamepadEvent::SDL_EVENT_GAMEPAD_BUTTON_UP:
      switch (gamepad_event.button_down.button) {
      case rc::SDL_GAMEPAD_BUTTON_LEFT_STICK:
      case rc::SDL_GAMEPAD_BUTTON_RIGHT_STICK:
      case rc::SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
      case rc::SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
        arming -= gamepad_event.button_down.button;
        break;
      }
      break;

    case rc::GamepadEvent::SDL_EVENT_GAMEPAD_AXIS_MOTION: {
      axis_positions[gamepad_event.axis_motion.axis] = gamepad_event.axis_motion.value;
      setBoardLED({
          constrain((uint8_t)map(axis_positions[0], INT16_MIN, INT16_MAX, 0, 255), (uint8_t)0, (uint8_t)255),
          constrain((uint8_t)map(axis_positions[1], INT16_MIN, INT16_MAX, 0, 255), (uint8_t)0, (uint8_t)255),
          0,
      });
      auto value = map(gamepad_event.axis_motion.value, INT16_MIN, INT16_MAX, crsf::CHANNEL_VALUE_MIN, crsf::CHANNEL_VALUE_MAX);
      switch (gamepad_event.axis_motion.axis) {
      case rc::SDL_GAMEPAD_AXIS_LEFTX:
        crsf_serial.channels[crsf::CHANNEL_RUDDER] = value;
        break;
      case rc::SDL_GAMEPAD_AXIS_LEFTY:
        crsf_serial.channels[crsf::CHANNEL_THROTTLE] = value;
        break;
      case rc::SDL_GAMEPAD_AXIS_RIGHTX:
        crsf_serial.channels[crsf::CHANNEL_AILERON] = value;
        break;
      case rc::SDL_GAMEPAD_AXIS_RIGHTY:
        crsf_serial.channels[crsf::CHANNEL_ELEVATOR] = value;
        break;
      }
    } break;
    }
  }

  if (report_timer.resetIfTicked()) {
    remote_event.type = rc::RemoteEvent::RC_EVENT_REPORT_LINK_STATS;
    memcpy(&remote_event.report_link_stats, &crsf_serial.link_stats, sizeof(crsf::LinkStatistics));
    RCGamepad::write(remote_event);

    remote_event.type = rc::RemoteEvent::RC_EVENT_REPORT_TELEMETRY;
    memcpy(&remote_event.report_telemetry, &crsf_serial.telemetry, sizeof(remote_event.report_telemetry));
    RCGamepad::write(remote_event);
  }
}
