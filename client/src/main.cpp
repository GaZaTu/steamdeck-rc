#include "BasicTimer.hpp"
#include "rc-protocol.hpp"
#include "serialib.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <chrono>
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

  BasicTimer _serial_timer{std::chrono::milliseconds{4}};

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
      _serial_timer.reset();

      _serial.writeBytes(_buffer.data(), _buffer_len * sizeof(rc::GamepadEvent));
      _buffer_len = 0;
    }
  }

  inline void read(rc::RemoteEvent& remote_event) {
    _serial.readBytes(&remote_event, sizeof(remote_event), 4);
  }

  inline bool available() {
    return _serial.available();
  }

  void getParameter(uint8_t parameter) {
    rc::GamepadEvent gamepad_event;
    gamepad_event.type = rc::GamepadEvent::PAD_EVENT_GET_PARAMETER;
    gamepad_event.get_parameter.parameter = parameter;
    write(gamepad_event);
  }

  void setParameter(uint8_t parameter, uint8_t value) {
    rc::GamepadEvent gamepad_event;
    gamepad_event.type = rc::GamepadEvent::PAD_EVENT_SET_PARAMETER;
    gamepad_event.set_parameter.parameter = parameter;
    gamepad_event.set_parameter.value = value;
    write(gamepad_event);
  }

  void requestAllConfigParameters() {
    getParameter(rc::PARAM_PACKET_RATE);
    getParameter(rc::PARAM_TLM_RATIO);
    getParameter(rc::PARAM_MAX_POWER);
    getParameter(rc::PARAM_LINK_MODE);
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
  std::string fontsize = "22";
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

    system(std::format("v4l2-ctl -d {} -v width=720,height=480", path).data());

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

static std::string_view elrs_packetrate_options[16] = {"50Hz", "100Hz", "150Hz", "250Hz", "333Hz", "500Hz", "D250Hz", "D500Hz", "F500Hz"};
static std::string_view elrs_tlmratio_options[16] = {"Std", "Off", "1:128", "1:64", "1:32", "1:16", "1:8", "1:4", "1:2"};
static std::string_view elrs_linkmode_options[16] = {"Normal", "MAVLink"};
static std::string_view elrs_txpower_options[16] = {"25mW", "50mW", "100mW", "250mW", "500mW", "1000mW"};

struct RCConfig {
  enum {
    INVALID = -1,
    // ELRS_CHANNEL,
    // VRX_CHANNEL,
    PACKET_RATE,
    TLM_RATIO,
    LINK_MODE,
    TX_POWER,
    // WIFI,
    COUNT,
  };

  bool visible = false;
  int8_t selection = 0;

  // uint8_t elrs_channel = 0;
  // uint8_t vrx_channel = 0;

  uint8_t packet_rate : 4 = 0xF;
  uint8_t tlm_ratio : 4 = 0xF;
  uint8_t link_mode : 4 = 0xF;
  uint8_t tx_power : 4 = 0xF;

  // uint8_t wifi = 0;

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
#define OPT(idx, name, value) \
  if (str.length())           \
    str += "\n";              \
  str += std::format(name "{}: {}", selection == idx ? '*' : ' ', value);

    std::string str;
    // OPT(elrs_channel, ELRS_CHANNEL);
    // OPT(vrx_channel, VRX_CHANNEL);
    OPT(PACKET_RATE, "packet_rate", elrs_packetrate_options[packet_rate]);
    OPT(TLM_RATIO, "tlm_ratio", elrs_tlmratio_options[tlm_ratio]);
    OPT(LINK_MODE, "link_mode", elrs_linkmode_options[link_mode]);
    OPT(TX_POWER, "tx_power", elrs_txpower_options[tx_power]);
    // OPT(wifi, WIFI);
    return str;

#undef OPT
  }

  RCOverlayText to_text() {
    return {to_string(), "20", "40", "red", "20", ":box=1:boxcolor=#00000070:boxborderw=4"};
  }

  void show(RCVideoPlayer& video) {
    video.setText("menu", to_text());
  }

  void showIfVisible(RCVideoPlayer& video) {
    if (visible) {
      show(video);
    }
  }

  void hide(RCVideoPlayer& video) {
    video.setText("menu", {});
  }

  static constexpr int8_t MOVE_LEFT = -1;
  static constexpr int8_t MOVE_RIGHT = +1;

  void changeValueOfSelection(RCBrain& brain, int8_t direction) {
    switch (selection) {
    // case ELRS_CHANNEL:
    //   brain.setELRSChannel(elrs_channel += direction);
    //   break;
    // case VRX_CHANNEL:
    //   brain.setVRXChannel(vrx_channel += direction);
    //   break;
    case PACKET_RATE:
      while (elrs_packetrate_options[packet_rate += direction] == "") {
      }
      brain.setParameter(rc::PARAM_PACKET_RATE, packet_rate);
      break;
    case TLM_RATIO:
      while (elrs_tlmratio_options[tlm_ratio += direction] == "") {
      }
      brain.setParameter(rc::PARAM_TLM_RATIO, tlm_ratio);
      break;
    case LINK_MODE:
      while (elrs_linkmode_options[link_mode += direction] == "") {
      }
      brain.setParameter(rc::PARAM_LINK_MODE, link_mode);
      break;
    case TX_POWER:
      while (elrs_txpower_options[tx_power += direction] == "") {
      }
      brain.setParameter(rc::PARAM_MAX_POWER, tx_power);
      break;
      // case WIFI:
      //   brain.setParameter(rc::PARAM_WIFI, wifi = !wifi);
      //   break;
    }
  }
};

constexpr auto SDL_GAMEPAD_MIN_DIFF = 256;
constexpr auto SDL_GAMEPAD_DEADZONE = 8192;

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

  video.setText("time", {"%{localtime}", "16", "h-th-16", "white", "14"});
  video.setText("vrx_rssi", {"vrx_rssi: 0", "480", "h-th-16"});

  if (!initializeSDL()) {
    printf("failed to initialize SDL\n");
    return 1;
  }

  brain.requestAllConfigParameters();

  rc::GamepadEvent gamepad_event;
  rc::RemoteEvent remote_event;

  SDL_Event event;

  int16_t axis_positions[rc::SDL_GAMEPAD_AXIS_COUNT] = {0};

  BasicTimer channel_timer{std::chrono::milliseconds{4}};

  bool running = true;
  while (running) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT:
        running = false;
        break;

      case SDL_EVENT_GAMEPAD_ADDED:
        printf("SDL_EVENT_GAMEPAD_ADDED\n");
        openFirstGamepad();
        brain.requestAllConfigParameters();
        break;

      case SDL_EVENT_GAMEPAD_REMOVED:
        printf("SDL_EVENT_GAMEPAD_REMOVED\n");
        break;

      case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        printf("SDL_EVENT_GAMEPAD_BUTTON_DOWN: %d\n", event.gbutton.button);
        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_START) {
          config.visible = !config.visible;
          if (config.visible) {
            if (config.packet_rate == 0xF) {
              brain.requestAllConfigParameters();
            }
            config.show(video);
          } else {
            config.hide(video);
            brain.requestAllConfigParameters();
          }
        } else {
          if (config.visible) {
            switch (event.gbutton.button) {
            case SDL_GAMEPAD_BUTTON_DPAD_UP:
              config.selection = std::max<int8_t>(config.selection - 1, RCConfig::INVALID + 1);
              config.show(video);
              break;
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
              config.selection = std::min<int8_t>(config.selection + 1, RCConfig::COUNT - 1);
              config.show(video);
              break;
            case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
              config.changeValueOfSelection(brain, RCConfig::MOVE_LEFT);
              config.show(video);
              break;
            case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
              config.changeValueOfSelection(brain, RCConfig::MOVE_RIGHT);
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
        if (!config.visible) {
          gamepad_event.type = SDL_EVENT_GAMEPAD_BUTTON_UP;
          gamepad_event.button_down.button = event.gbutton.button;
          brain.write(gamepad_event);
        }
        break;

      case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        if (std::abs(event.gaxis.value) < SDL_GAMEPAD_DEADZONE) {
          event.gaxis.value = 0;
        } else {
          if (event.gaxis.value > 0) {
            event.gaxis.value -= SDL_GAMEPAD_DEADZONE;
          } else {
            event.gaxis.value += SDL_GAMEPAD_DEADZONE;
          }
        }
        axis_positions[event.gaxis.axis] = event.gaxis.value;
        // if (std::abs(std::abs(event.gaxis.value) - std::abs(axis_positions[event.gaxis.axis])) > SDL_GAMEPAD_MIN_DIFF) {
        //   axis_positions[event.gaxis.axis] = event.gaxis.value;
        //   // printf("SDL_EVENT_GAMEPAD_AXIS_MOTION: %d, %d\n", event.gaxis.axis, event.gaxis.value);
        //   // video.setText(std::format("axis{}", event.gaxis.axis), {std::format("axis{}: {}", event.gaxis.axis, event.gaxis.value), "w-tw-8", std::format("h-{}", 32 * (event.gaxis.axis + 2))});
        //   gamepad_event.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
        //   gamepad_event.axis_motion.axis = event.gaxis.axis;
        //   gamepad_event.axis_motion.value = event.gaxis.value;
        //   brain.write(gamepad_event);
        // }
        break;
      }
    }

    if (channel_timer.resetIfTicked()) {
      for (int i = 0; i < 4; i++) {
        gamepad_event.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
        gamepad_event.axis_motion.axis = i;
        gamepad_event.axis_motion.value = axis_positions[i];
        brain.write(gamepad_event);
      }
    }

    if (brain.available()) {
      brain.read(remote_event);
      switch (remote_event.type) {
      case rc::RemoteEvent::RC_EVENT_REPORT_LINK_STATS: {
        auto& l = remote_event.report_link_stats;
        video.setText("link_stats", {
          std::format("rssi[up]: {}\n lqi[up]: {}\n snr[up]: {}\nrate[up]: {}\npowr[up]: {}\nrssi[dn]: {}\n lqi[dn]: {}\n snr[dn]: {}",
            (l.up_rssi_ant1 + l.up_rssi_ant2) / 2, l.up_link_quality, l.up_snr, l.rf_profile, l.up_rf_power, l.down_rssi, l.down_link_quality, l.down_snr
          ),
          "w-240", "h-th-16"
        });
        // printf("stat: rssi1=-%ddBm rssi2=-%ddBm lqi=%d%% snr=%ddB ant=%d rate=%dhz power=%dmW d_rssi=-%ddBm d_lqi=%d%% d_snr=%ddB\n", l.up_rssi_ant1, l.up_rssi_ant2, l.up_link_quality, l.up_snr, l.active_antenna, l.rf_profile, l.up_rf_power, l.down_rssi, l.down_link_quality, l.down_snr);
      } break;

      case rc::RemoteEvent::RC_EVENT_REPORT_TELEMETRY: {
        auto& t = remote_event.report_telemetry;
        // video.setText("attitude", {
        //   std::format(""),
        //   "w-240", "h-th-16"
        // });
      } break;

      case rc::RemoteEvent::RC_EVENT_REPORT_ARMED:
        printf("RC_EVENT_REPORT_ARMED: %d\n", remote_event.report_armed.armed);
        break;

      case rc::RemoteEvent::RC_EVENT_REPORT_PARAMETER:
        switch (remote_event.report_parameter.parameter) {
        case rc::PARAM_PACKET_RATE:
          printf("PARAM_PACKET_RATE = %d\n", remote_event.report_parameter.value);
          config.packet_rate = remote_event.report_parameter.value;
          config.showIfVisible(video);
          break;
        case rc::PARAM_TLM_RATIO:
          printf("PARAM_TLM_RATIO = %d\n", remote_event.report_parameter.value);
          config.tlm_ratio = remote_event.report_parameter.value;
          config.showIfVisible(video);
          break;
        case rc::PARAM_LINK_MODE:
          printf("PARAM_LINK_MODE = %d\n", remote_event.report_parameter.value);
          config.link_mode = remote_event.report_parameter.value;
          config.showIfVisible(video);
          break;
        case rc::PARAM_MAX_POWER:
          printf("PARAM_POWER = %d\n", remote_event.report_parameter.value);
          config.tx_power = remote_event.report_parameter.value;
          config.showIfVisible(video);
          break;
        case rc::PARAM_WIFI:
          printf("PARAM_WIFI = %d\n", remote_event.report_parameter.value);
          break;
        }
        break;

      // case rc::RemoteEvent::RC_EVENT_REPORT_PARAMETER:
      //   printf("RC_EVENT_REPORT_PARAMETER: %d\n", remote_event.report_vrx_channel.channel);
      //   break;

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
