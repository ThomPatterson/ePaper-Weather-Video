#ifndef HTTPCONTROLLER_H
#define HTTPCONTROLLER_H

#include <WiFi.h>
#include <HTTPClient.h>
#include "DisplayController.h"


class HttpController {
private:
  DisplayController* displayController;
  void logWithTimestamp(const String& message);
  float readBatteryVoltage();
public:
  void init(DisplayController* displayController);
  void fetchImage(uint8_t* image);
  void connectWiFi();
  void disconnectWiFi();
};

#endif