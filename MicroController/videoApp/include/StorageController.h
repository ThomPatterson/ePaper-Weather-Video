#ifndef STORAGECONTROLLER_H
#define STORAGECONTROLLER_H

#include <FS.h>
#include <LittleFS.h>
#include "DisplayController.h"
#include <string>


class StorageController {
private:
  DisplayController* displayController;
  std::string zeroPad(int num, int size);
  void logWithTimestamp(const String& message);
public:
  void init(DisplayController* displayController);
  void showAvailableSpace();
  void purgeCache();
  bool cacheHasImage();
  int getLargestFileNumber();
  int getSmallestFileNumber();
  int getCacheSize();
  bool cacheHasRoomForAnotherImage();
  void writeImageToCache(uint8_t* image);
  bool getNextImage(uint8_t* image);
};

#endif