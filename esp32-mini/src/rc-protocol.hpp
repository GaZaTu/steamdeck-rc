#pragma once

#include "crsf-structs.hpp"
#include <cstdint>

namespace rc {
constexpr auto CDC_PACKET_SIZE = 64;

typedef enum SDL_GamepadButton {
  SDL_GAMEPAD_BUTTON_INVALID = -1,
  SDL_GAMEPAD_BUTTON_SOUTH, /**< Bottom face button (e.g. Xbox A button) */
  SDL_GAMEPAD_BUTTON_EAST,  /**< Right face button (e.g. Xbox B button) */
  SDL_GAMEPAD_BUTTON_WEST,  /**< Left face button (e.g. Xbox X button) */
  SDL_GAMEPAD_BUTTON_NORTH, /**< Top face button (e.g. Xbox Y button) */
  SDL_GAMEPAD_BUTTON_BACK,
  SDL_GAMEPAD_BUTTON_GUIDE,
  SDL_GAMEPAD_BUTTON_START,
  SDL_GAMEPAD_BUTTON_LEFT_STICK,
  SDL_GAMEPAD_BUTTON_RIGHT_STICK,
  SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
  SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
  SDL_GAMEPAD_BUTTON_DPAD_UP,
  SDL_GAMEPAD_BUTTON_DPAD_DOWN,
  SDL_GAMEPAD_BUTTON_DPAD_LEFT,
  SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
  SDL_GAMEPAD_BUTTON_MISC1, /**< Additional button (e.g. Xbox Series X share button, PS5 microphone button, Nintendo
                               Switch Pro capture button, Amazon Luna microphone button, Google Stadia capture button)
                             */
  SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1, /**< Upper or primary paddle, under your right hand (e.g. Xbox Elite paddle P1,
                                       DualSense Edge RB button, Right Joy-Con SR button) */
  SDL_GAMEPAD_BUTTON_LEFT_PADDLE1,  /**< Upper or primary paddle, under your left hand (e.g. Xbox Elite paddle P3,
                                       DualSense Edge LB button, Left Joy-Con SL button) */
  SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2, /**< Lower or secondary paddle, under your right hand (e.g. Xbox Elite paddle P2,
                                       DualSense Edge right Fn button, Right Joy-Con SL button) */
  SDL_GAMEPAD_BUTTON_LEFT_PADDLE2,  /**< Lower or secondary paddle, under your left hand (e.g. Xbox Elite paddle P4,
                                       DualSense Edge left Fn button, Left Joy-Con SR button) */
  SDL_GAMEPAD_BUTTON_TOUCHPAD,      /**< PS4/PS5 touchpad button */
  SDL_GAMEPAD_BUTTON_MISC2,         /**< Additional button */
  SDL_GAMEPAD_BUTTON_MISC3,         /**< Additional button (e.g. Nintendo GameCube left trigger click) */
  SDL_GAMEPAD_BUTTON_MISC4,         /**< Additional button (e.g. Nintendo GameCube right trigger click) */
  SDL_GAMEPAD_BUTTON_MISC5,         /**< Additional button */
  SDL_GAMEPAD_BUTTON_MISC6,         /**< Additional button */
  SDL_GAMEPAD_BUTTON_COUNT,
} SDL_GamepadButton;

typedef enum SDL_GamepadAxis {
  SDL_GAMEPAD_AXIS_INVALID = -1,
  SDL_GAMEPAD_AXIS_LEFTX,
  SDL_GAMEPAD_AXIS_LEFTY,
  SDL_GAMEPAD_AXIS_RIGHTX,
  SDL_GAMEPAD_AXIS_RIGHTY,
  SDL_GAMEPAD_AXIS_LEFT_TRIGGER,
  SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,
  SDL_GAMEPAD_AXIS_COUNT,
} SDL_GamepadAxis;

enum Parameter {
  // elrs
  PARAM_PACKET_RATE = elrs::PARAM_PACKET_RATE,
  PARAM_TLM_RATIO = elrs::PARAM_TLM_RATIO,
  PARAM_SWITCH_MODE = elrs::PARAM_SWITCH_MODE,
  PARAM_LINK_MODE = elrs::PARAM_LINK_MODE,
  PARAM_MODEL_MATCH = elrs::PARAM_MODEL_MATCH,
  PARAM_MAX_POWER = elrs::PARAM_MAX_POWER,
  PARAM_WIFI = elrs::PARAM_WIFI,
  // custom
  PARAM_VRX_FREQ = 0xC0,
};

struct [[gnu::packed]] GamepadEvent {
  enum Type {
    PAD_EVENT_GET_PARAMETER,
    PAD_EVENT_SET_PARAMETER,
    SDL_EVENT_GAMEPAD_AXIS_MOTION = 1616,
    SDL_EVENT_GAMEPAD_BUTTON_DOWN = 1617,
    SDL_EVENT_GAMEPAD_BUTTON_UP = 1618,
  };

  uint16_t type;
  union {
    struct [[gnu::packed]] {
      uint8_t parameter;
    } get_parameter;
    struct [[gnu::packed]] {
      uint8_t parameter;
      uint8_t value;
    } set_parameter;
    struct [[gnu::packed]] {
      uint8_t axis;
      int16_t value;
    } axis_motion;
    struct [[gnu::packed]] {
      uint8_t button;
    } button_down;
    struct [[gnu::packed]] {
      uint8_t button;
    } button_up;
  };
};

static_assert(sizeof(GamepadEvent) < CDC_PACKET_SIZE);

struct [[gnu::packed]] RemoteEvent {
  enum Type {
    RC_EVENT_REPORT_LINK_STATS,
    RC_EVENT_REPORT_TELEMETRY,
    RC_EVENT_REPORT_ARMED,
    RC_EVENT_REPORT_PARAMETER,
    RC_EVENT_REPORT_VRX_CHANNEL,
    RC_EVENT_REPORT_VRX_RSSI,
  };

  uint16_t type;
  union {
    crsf::LinkStatistics report_link_stats;
    crsf::Telemetry report_telemetry;
    struct [[gnu::packed]] {
      uint8_t armed;
    } report_armed;
    struct [[gnu::packed]] {
      uint8_t parameter;
      uint8_t value;
    } report_parameter;
    struct [[gnu::packed]] {
      uint8_t channel;
    } report_vrx_channel;
    struct [[gnu::packed]] {
      uint8_t percent;
    } report_vrx_rssi;
  };
};

static_assert(sizeof(RemoteEvent) < CDC_PACKET_SIZE);
} // namespace rc
