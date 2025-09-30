#include "BasicTimer.hpp"
#include "rc-protocol.hpp"
#include "serialib.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <mpv/client.h>
#include <string>
#include <unordered_map>

inline auto get_executable_path() {
  return std::filesystem::canonical("/proc/self/exe");
}

class RCBrain {
private:
  serialib _serial;

  rc::GamepadEvent _gamepad_event;

  std::array<rc::GamepadEvent, rc::CDC_PACKET_SIZE / sizeof(rc::GamepadEvent)> _buffer;
  uint8_t _buffer_len = 0;

  BasicTimer _serial_timer{std::chrono::milliseconds{5}};

public:
  bool open(std::string_view path = "", uint32_t baud = 115200) {
    if (path.empty()) {
      for (auto i = 0; i < 99; i++) {
        auto device_name = std::format("/dev/ttyACM{}", i);
        if (_serial.openDevice(device_name.data(), baud) == 1) {
          return true;
        }
      }
      return false;
    } else {
      return _serial.openDevice(path.data(), baud) == 1;
    }
  }

  inline void write(const rc::GamepadEvent& gamepad_event) {
    _buffer[_buffer_len++] = gamepad_event;

    if (_serial_timer.hasTicked() || _buffer_len == _buffer.size()) {
      _serial.writeBytes(_buffer.data(), _buffer_len * sizeof(rc::GamepadEvent));

      _buffer_len = 0;
      _serial_timer.reset();
    }
  }

  inline void read(rc::RemoteEvent& remote_event) {
    _serial.readBytes(&remote_event, sizeof(remote_event), 5);
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
  std::string fontcolor = "white";
  std::string fontsize = "32";
  std::string more = "";

  std::string to_string() {
    return std::format("drawtext=text=\"{}\":x=({}):y=({}):fontcolor={}:fontsize={}:font=Mono{}:borderw=2", str, x, y, fontcolor, fontsize, more);
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

  bool begin(std::string_view path = "") {
    if (path.empty()) {
      // TODO: auto select /dev/video{}
      return true;
    }

    system(std::format("v4l2-ctl -d {} -v width=1280,height=720", path).data());

    _mpv = mpv_create();

    mpv_set_property_string(_mpv, "profile", "low-latency");
    mpv_set_property_string(_mpv, "untimed", "");

    mpv_initialize(_mpv);

    const char* cmd[] = {"loadfile", path.data(), nullptr};
    return mpv_command(_mpv, cmd);
  }

  mpv_event* pollEvent() {
    return mpv_wait_event(_mpv, 0);
  }

  void setText(std::string_view key, RCOverlayText&& text) {
    _overlay[std::string{key}] = std::move(text);
    updateOverlay();
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

  std::string serial_device;
  std::string video_device;

  void loadDevicePathsFromFile() {
#define OPT(name)            \
  if (key == #name) {        \
    name = std::move(value); \
  }

    auto dir = get_executable_path().parent_path();
    auto cfg = dir / "devices.cfg";

    std::ifstream cfg_stream{cfg};
    std::string line;
    while (std::getline(cfg_stream, line)) {
      auto key = line.substr(0, line.find_first_of('='));
      auto value = line.substr(line.find_first_of('=') + 1);

      OPT(serial_device);
      OPT(video_device);
    }

#undef OPT
  }

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
    return {to_string(), "80", "240", "red", "32", ":box=1:boxcolor=#00000070:boxborderw=4"};
  }

  void show(RCVideoPlayer& video) {
    video.setText("menu", to_text());
  }

  void hide(RCVideoPlayer& video) {
    video.setText("menu", {});
  }
};

constexpr auto SDL_GAMEPAD_MIN_DIFF = 256;
constexpr auto SDL_GAMEPAD_DEADZONE = 2048;

int main(int argc, char** argv) {
  RCConfig config;
  config.loadDevicePathsFromFile();

  RCBrain brain;
  if (brain.open(config.serial_device)) {
    printf("opened serial device at %s\n", config.serial_device.data());
  }

  RCVideoPlayer video;
  if (video.begin(config.video_device)) {
    printf("opened video device at %s\n", config.video_device.data());
  }

  video.setText("time", {"%{localtime}", "16", "h-th-16", "white", "20"});
  video.setText("vrx_rssi", {"vrx_rssi: 0", "480", "h-th-16"});

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
            config.show(video);
          } else {
            config.hide(video);
          }
        } else {
          if (config.visible) {
            switch (event.gbutton.button) {
            case SDL_GAMEPAD_BUTTON_DPAD_UP:
              config.selection = std::max<int8_t>(config.selection - 1, RCConfig::INVALID);
              config.show(video);
              break;
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
              config.selection = std::min<int8_t>(config.selection + 1, RCConfig::COUNT);
              config.show(video);
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
              config.show(video);
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
              config.show(video);
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
      case rc::RemoteEvent::RC_EVENT_REPORT_ELRS_CHANNEL:
        printf("RC_EVENT_NOTIFY_ELRS_CHANNEL: %d\n", remote_event.report_elrs_channel.channel);
        break;

      case rc::RemoteEvent::RC_EVENT_REPORT_VRX_CHANNEL:
        printf("RC_EVENT_NOTIFY_VRX_CHANNEL: %d\n", remote_event.report_vrx_channel.channel);
        break;

      case rc::RemoteEvent::RC_EVENT_REPORT_VRX_RSSI:
        printf("RC_EVENT_NOTIFY_VRX_RSSI: %d%%\n", remote_event.report_vrx_rssi.percent);
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
