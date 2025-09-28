#include <Arduino.h>
#include "esp32mini.hpp"

void setup() {
  Serial.begin(SERIAL_BAUD);

  setBoardLED({0, 255, 0});

  delay(3000);
  Serial.printf("fdm1\n");
  LOG("FDM");
  Serial.printf("fdm2\n");

  setBoardLED({255, 0, 255});
}

void loop() {
  if (Serial.available()) {
    auto str = Serial.readStringUntil('\n');
    Serial.printf("rx: %s\n", str.c_str());
  }
}
