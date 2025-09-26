#ifndef DISPLAYCONTROLLER_H
#define DISPLAYCONTROLLER_H

#include <GxEPD2.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_EPD.h>



class DisplayController {
public:
  void init();
  void displayImage(const uint8_t* image);
  void displayText(const char* message);
  void clearMessages();
  void showMessage(const char* message);
  void dismissMessage();
  void powerDown();
private: 
  uint8_t* rotateImage180(const uint8_t* image, uint16_t width, uint16_t height);
  const uint8_t* lastImage;
  void logWithTimestamp(const String& message);
};

#endif