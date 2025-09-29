#include "rc-protocol.hpp"
#include "serialib.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <mpv/client.h>
#include <string>
#include <unordered_map>

class RCBrain {
private:
  serialib _serial;

  rc::GamepadEvent _gamepad_event;

public:
  bool open(std::string_view path = "") {
    if (path.empty()) {
      for (auto i = 0; i < 99; i++) {
        auto device_name = std::format("/dev/ttyACM{}", i);
        if (_serial.openDevice(device_name.data(), 115200) == 1) {
          return true;
        }
      }
      return false;
    } else {
      return _serial.openDevice(path.data(), 115200) == 1;
    }
  }

  inline void write(const rc::GamepadEvent& gamepad_event) {
    _serial.writeBytes(&gamepad_event, sizeof(gamepad_event));
  }

  inline void read(rc::RemoteEvent& remote_event) {
    _serial.readBytes(&remote_event, sizeof(remote_event), 10);
  }

  inline bool available() {
    return _serial.available();
  }

  inline void setELRSChannel(uint8_t channel) {
    rc::GamepadEvent gamepad_event;
    gamepad_event.type = rc::GamepadEvent::PAD_EVENT_SET_ELRS_CHANNEL;
    gamepad_event.set_elrs_channel.channel = channel;
    write(gamepad_event);
  }

  inline void setVRXChannel(uint8_t channel) {
    rc::GamepadEvent gamepad_event;
    gamepad_event.type = rc::GamepadEvent::PAD_EVENT_SET_VRX_CHANNEL;
    gamepad_event.set_vrx_channel.channel = channel;
    write(gamepad_event);
  }
};

bool openFirstGamepad() {
  auto gamepad_count = 0;
  auto gamepads = SDL_GetGamepads(&gamepad_count);
  if (!gamepad_count) {
    printf("gamepad_count: %d\n", gamepad_count);
    return false;
  }

  auto gamepad = SDL_OpenGamepad(*gamepads);
  if (!gamepad) {
    printf("SDL_OpenGamepad failed: %s\n", SDL_GetError());
    return false;
  }

  return true;
}

bool initializeSDL() {
  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
  SDL_SetHint("SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD", "1");

  if (!SDL_Init(SDL_INIT_GAMEPAD)) {
    printf("SDL_Init failed: %s\n", SDL_GetError());
    return false;
  }

  openFirstGamepad();

  return true;
}

struct RCOverlayText {
  std::string str;
  std::string x;
  std::string y;
  std::string fontcolor = "red";
  std::string fontsize = "32";

  std::string to_string() {
    return std::format("drawtext=text=\"{}\":x=({}):y=({}):fontcolor={}:fontsize={}:font=Mono", str, x, y, fontcolor, fontsize);
  }
};

struct RCConfig {
  enum {
    INVALID = -1,
    ELRS_CHANNEL,
    VRX_CHANNEL,
    COUNT,
  };

  bool visible = false;
  int8_t selection = -1;

  uint8_t elrs_channel = 0;
  uint8_t vrx_channel = 0;

  std::string to_string() {
#define OPT(name, idx) \
  if (str.length())    \
    str += "\n";       \
  str += std::format(#name "{}: {}", selection == idx ? '*' : ' ', name);

    std::string str;
    OPT(elrs_channel, ELRS_CHANNEL);
    OPT(vrx_channel, VRX_CHANNEL);
    return str;
#undef OPT
  }

  RCOverlayText to_text() {
    return {to_string(), "80", "240"};
  }
};

class RCVideoPlayer {
private:
  mpv_handle* _mpv = nullptr;

  std::unordered_map<std::string, RCOverlayText> _overlay;

  std::string _vf;
  void updateOverlay() {
    _vf = "";
    for (auto& [key, text] : _overlay) {
      if (text.str.empty()) {
        continue;
      }
      if (_vf.length()) {
        _vf += ",";
      }
      _vf += text.to_string();
    }

    mpv_set_property_string(_mpv, "vf", _vf.data());
  }

public:
  ~RCVideoPlayer() {
    if (_mpv) {
      mpv_terminate_destroy(_mpv);
    }
  }

  void begin(std::string_view path = "") {
    if (path.empty()) {
      // TODO: auto select /dev/video{}
      return;
    }

    system(std::format("v4l2-ctl -d {} -v width=1280,height=720", path).data());

    _mpv = mpv_create();

    mpv_set_property_string(_mpv, "profile", "low-latency");
    mpv_set_property_string(_mpv, "untimed", "");

    mpv_initialize(_mpv);

    const char* cmd[] = {"loadfile", path.data(), nullptr};
    mpv_command(_mpv, cmd);
  }

  mpv_event* pollEvent() {
    return mpv_wait_event(_mpv, 0);
  }

  void setText(std::string_view key, RCOverlayText&& text) {
    _overlay[std::string{key}] = std::move(text);
    updateOverlay();
  }
};

constexpr auto SDL_GAMEPAD_MIN_DIFF = 256;
constexpr auto SDL_GAMEPAD_DEADZONE = 2048;

int main(int argc, char** argv) {
  RCConfig config;

  RCBrain brain;
  brain.open("/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_24:EC:4A:10:39:BC-if00");
  printf("opened serial device\n");

  RCVideoPlayer video;
  video.begin("/dev/video0");

  if (!initializeSDL()) {
    printf("failed to initialize SDL\n");
    return 1;
  }

  rc::GamepadEvent gamepad_event;
  rc::RemoteEvent remote_event;

  SDL_Event event;

  int16_t axis_positions[rc::SDL_GAMEPAD_AXIS_COUNT] = {0};

  auto running = true;
  while (running) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT:
        running = false;
        break;

      case SDL_EVENT_GAMEPAD_ADDED:
        printf("SDL_EVENT_GAMEPAD_ADDED\n");
        openFirstGamepad();
        break;

      case SDL_EVENT_GAMEPAD_REMOVED:
        printf("SDL_EVENT_GAMEPAD_REMOVED\n");
        break;

      case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        printf("SDL_EVENT_GAMEPAD_BUTTON_DOWN: %d\n", event.gbutton.button);
        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_START) {
          config.visible = !config.visible;
          if (config.visible) {
            video.setText("menu", config.to_text());
          } else {
            video.setText("menu", {});
          }
        } else {
          if (config.visible) {
            switch (event.gbutton.button) {
            case SDL_GAMEPAD_BUTTON_DPAD_UP:
              config.selection = std::max<int8_t>(config.selection - 1, RCConfig::INVALID);
              video.setText("menu", config.to_text());
              break;
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
              config.selection = std::min<int8_t>(config.selection + 1, RCConfig::COUNT);
              video.setText("menu", config.to_text());
              break;
            case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
              switch (config.selection) {
              case RCConfig::ELRS_CHANNEL:
                brain.setELRSChannel(config.elrs_channel -= 1);
                break;
              case RCConfig::VRX_CHANNEL:
                brain.setVRXChannel(config.vrx_channel -= 1);
                break;
              }
              video.setText("menu", config.to_text());
              break;
            case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
              switch (config.selection) {
              case RCConfig::ELRS_CHANNEL:
                brain.setELRSChannel(config.elrs_channel += 1);
                break;
              case RCConfig::VRX_CHANNEL:
                brain.setVRXChannel(config.vrx_channel += 1);
                break;
              }
              video.setText("menu", config.to_text());
              break;
            }
          } else {
            gamepad_event.type = SDL_EVENT_GAMEPAD_BUTTON_DOWN;
            gamepad_event.button_down.button = event.gbutton.button;
            brain.write(gamepad_event);
          }
        }
        break;

      case SDL_EVENT_GAMEPAD_BUTTON_UP:
        printf("SDL_EVENT_GAMEPAD_BUTTON_UP: %d\n", event.gbutton.button);
        break;

      case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        if (std::abs(event.gaxis.value) < SDL_GAMEPAD_DEADZONE) {
          event.gaxis.value = 0;
        }
        if (std::abs(std::abs(event.gaxis.value) - std::abs(axis_positions[event.gaxis.axis])) > SDL_GAMEPAD_MIN_DIFF) {
          axis_positions[event.gaxis.axis] = event.gaxis.value;
          // printf("SDL_EVENT_GAMEPAD_AXIS_MOTION: %d, %d\n", event.gaxis.axis, event.gaxis.value);
          video.setText(std::format("axis{}", event.gaxis.axis), {std::format("axis{}: {}", event.gaxis.axis, event.gaxis.value), "w-tw-8", std::format("h-{}", 32 * (event.gaxis.axis + 2))});
          gamepad_event.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
          gamepad_event.axis_motion.axis = event.gaxis.axis;
          gamepad_event.axis_motion.value = event.gaxis.value;
          brain.write(gamepad_event);
        }
        break;
      }
    }

    if (brain.available()) {
      brain.read(remote_event);
      switch (remote_event.type) {
      case rc::RemoteEvent::RC_EVENT_NOTIFY_ELRS_CHANNEL:
        printf("RC_EVENT_NOTIFY_ELRS_CHANNEL: %d\n", remote_event.notify_elrs_channel.channel);
        break;

      case rc::RemoteEvent::RC_EVENT_NOTIFY_VRX_CHANNEL:
        printf("RC_EVENT_NOTIFY_VRX_CHANNEL: %d\n", remote_event.notify_vrx_channel.channel);
        break;

      case rc::RemoteEvent::RC_EVENT_NOTIFY_VRX_RSSI:
        printf("RC_EVENT_NOTIFY_VRX_RSSI: %d%%\n", remote_event.notify_vrx_rssi.percent);
        break;
      }
    }

    auto mpv_event = video.pollEvent();
    if (mpv_event) {
      if (mpv_event->event_id == MPV_EVENT_SHUTDOWN) {
        running = false;
        continue;
      }
    }
  }
}
