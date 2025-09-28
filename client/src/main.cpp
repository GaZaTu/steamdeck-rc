#include "serialib.h"
#include <chrono>
#include <cstdio>
#include <format>
#include <libgamepad.hpp>
#include <thread>

int main(int argc, char** argv) {
  auto device_name = std::format("/dev/ttyACM{}", 0);

  serialib serial;
  if (serial.openDevice(device_name.data(), 115200) != 1) {
    printf("failed to open serial device\n");
    return 1;
  }

  printf("opened serial device %d\n", serial.available());
  printf("cts:%d, dcd:%d, dsr:%d, dtr:%d, ri:%d, rts:%d\n",
    serial.isCTS(),
    serial.isDCD(),
    serial.isDSR(),
    serial.isDTR(),
    serial.isRI(),
    serial.isRTS()
  );


    auto gpad = gamepad::hook::make();
    gpad->set_plug_and_play(true, std::chrono::milliseconds(10000));

    std::atomic<bool> run_flag = true;

    gpad->set_button_event_handler([&](std::shared_ptr<gamepad::device> pad) {
      ginfo("Received button event: Native id: %i, Virtual id: %i val: %i", pad->last_button_event()->native_id, pad->last_button_event()->vc, pad->last_button_event()->value);

      if (pad->is_button_pressed(gamepad::button::Y)) {
        printf("Y is pressed, exiting\n");
        run_flag = false;
      }
    });

    gpad->set_axis_event_handler([&](std::shared_ptr<gamepad::device> pad) {
      ginfo("Received axis event: Native id: %i, Virtual id: %i val: %i", pad->last_axis_event()->native_id, pad->last_axis_event()->vc, pad->last_axis_event()->value);
    });

    if (!gpad->start()) {
      printf("Failed to start hook\n");
      return 1;
    }

    while (run_flag) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

  // serial.writeString("test\n");

  // std::string buffer;
  // while (true) {
  //   buffer.resize(256);
  //   buffer.resize(serial.readString(buffer.data(), '\n', buffer.size()));

  //   printf("POG? %s\n", buffer.data());
  // }
}
