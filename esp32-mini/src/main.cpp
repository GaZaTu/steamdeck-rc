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

void setup() {
  Serial.begin(SERIAL_BAUD);

  setBoardLED({0, 255, 0});
  delay(1000);
  setBoardLED({255, 0, 255});
}

static rc::GamepadEvent gamepad_event;
static rc::RemoteEvent remote_event;

static int16_t axis_positions[rc::SDL_GAMEPAD_AXIS_COUNT] = {0};

void loop() {
  if (RCGamepad::available()) {
    RCGamepad::read(gamepad_event);
    switch (gamepad_event.type) {
    case rc::GamepadEvent::PAD_EVENT_SET_ELRS_CHANNEL:
      remote_event.type = rc::RemoteEvent::RC_EVENT_NOTIFY_ELRS_CHANNEL;
      remote_event.notify_elrs_channel.channel = gamepad_event.button_down.button;
      RCGamepad::write(remote_event);
      break;

    case rc::GamepadEvent::PAD_EVENT_SET_VRX_CHANNEL:
      remote_event.type = rc::RemoteEvent::RC_EVENT_NOTIFY_VRX_CHANNEL;
      remote_event.notify_vrx_channel.channel = gamepad_event.button_down.button;
      RCGamepad::write(remote_event);
      break;

    case rc::GamepadEvent::SDL_EVENT_GAMEPAD_BUTTON_DOWN:
      break;

    case rc::GamepadEvent::SDL_EVENT_GAMEPAD_AXIS_MOTION:
      axis_positions[gamepad_event.axis_motion.axis] = gamepad_event.axis_motion.value;
      setBoardLED({
        constrain((uint8_t)map(axis_positions[0], INT16_MIN, INT16_MAX, 0, 255), (uint8_t)0, (uint8_t)255),
        constrain((uint8_t)map(axis_positions[1], INT16_MIN, INT16_MAX, 0, 255), (uint8_t)0, (uint8_t)255),
        0,
      });
      break;
    }
  }
}
