#include "BasicTimer.hpp"
#include "esp32mini.hpp"
#include <Arduino.h>
#include <CRSFforArduino.hpp>

CRSFforArduino crsf{&Serial0, 3, 1};

void setup() {
  Serial.begin(SERIAL_BAUD);

  setBoardLED({255, 0, 0});
  delay(2000);

  if (!crsf.begin()) {
    Serial.printf("crsf.begin() failed\n");
    crsf.end();
  } else {
    Serial.printf("crsf.begin() success\n");
    // crsf.setLinkStatisticsCallback([](serialReceiverLayer::link_statistics_t linkStatistics) {
    //   Serial.printf("link: rssi=%d lqi=%d snr=%d power=%d\n", linkStatistics.rssi, linkStatistics.lqi, linkStatistics.snr, linkStatistics.tx_power);
    // });
    crsf.setRcChannelsCallback([](serialReceiverLayer::rcChannels_t* channels) {
      static unsigned long lastPrint = millis();
      if (millis() - lastPrint >= 100) {
        lastPrint = millis();

        static bool initialised = false;
        static bool lastFailSafe = false;
        if (channels->failsafe != lastFailSafe || !initialised) {
          initialised = true;
          lastFailSafe = channels->failsafe;
          Serial.print("FailSafe: ");
          Serial.println(lastFailSafe ? "Active" : "Inactive");
        }

        if (channels->failsafe == false) {
          Serial.print("RC Channels <A: ");
          Serial.print(crsf.rcToUs(channels->value[0]));
          Serial.print(", E: ");
          Serial.print(crsf.rcToUs(channels->value[1]));
          Serial.print(", T: ");
          Serial.print(crsf.rcToUs(channels->value[2]));
          Serial.print(", R: ");
          Serial.print(crsf.rcToUs(channels->value[3]));
          Serial.print(", Aux1: ");
          Serial.print(crsf.rcToUs(channels->value[4]));
          Serial.print(", Aux2: ");
          Serial.print(crsf.rcToUs(channels->value[5]));
          Serial.print(", Aux3: ");
          Serial.print(crsf.rcToUs(channels->value[6]));
          Serial.print(", Aux4: ");
          Serial.print(crsf.rcToUs(channels->value[7]));
          Serial.println(">");

          setBoardLED({
              (uint8_t)map(crsf.rcToUs(channels->value[3]), 988, 2012, 0, 255),
              (uint8_t)map(crsf.rcToUs(channels->value[2]), 988, 2012, 0, 255),
              0,
          });
        }
      }
    });
  }

  setBoardLED({0, 255, 0});
}

static BasicTimer print_timer{3000};

void loop() {
  crsf.update();
}
