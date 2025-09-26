#include "Config.h"
#include "StorageController.h"
#include "HttpController.h"
#include "DisplayController.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include <esp_sleep.h>
#include <esp_system.h>

#define PURGE_CACHE_BUTTON 2

// image array for fetched images
uint8_t image[imageBytes];

StorageController storageController;
HttpController httpController;
DisplayController displayController;
int failCount;

void logWithTimestamp(const String& message) {
  // Get the number of milliseconds since the device started
  unsigned long currentTime = millis();
  unsigned long seconds = currentTime / 1000;
  unsigned long milliseconds = currentTime % 1000;
  Serial.println("[" + String(seconds) + "." + String(milliseconds) + "] " + message);
}


/*
* Set the boot partition back to the selector app for the next boot (wake)
*/
bool resetBootPartition() {
  // Find the factory partition (selector app)
  const esp_partition_t* factory_partition = esp_partition_find_first(
    ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
  
  if (factory_partition != NULL) {
    logWithTimestamp(String("Found factory partition at address 0x") + String(factory_partition->address, HEX));
    
    // Set boot partition
    esp_err_t err = esp_ota_set_boot_partition(factory_partition);
    if (err == ESP_OK) {
      //logWithTimestamp("Boot partition set to selector app");
      return true;
    } else {
      logWithTimestamp(String("Failed to set boot partition, error: ") + String(err));
      return false;
    }
  } else {
    logWithTimestamp("ERROR: Factory partition not found!");
    return false;
  }
}


void displayNumberOfImagesInCache() {
  int cacheSize = storageController.getCacheSize();
  std::string cacheSizeStr = std::to_string(cacheSize);
  const char* numberCStr = cacheSizeStr.c_str();
  displayController.showMessage((String("Images in cache: ") + numberCStr).c_str());
}

void populateCache() {
  httpController.connectWiFi();
  while (storageController.cacheHasRoomForAnotherImage()) {
    httpController.fetchImage(image);
    storageController.writeImageToCache(image);
  }
  httpController.disconnectWiFi();
}

void goToDeepSleep() {
  logWithTimestamp("MainController: Going to deep sleep.");
  esp_sleep_enable_timer_wakeup(deepSleepTime);  // Time in microseconds
  displayController.powerDown();                 //power down the display
  esp_deep_sleep_start();                        // Enter deep sleep mode
}

void setup() {
  Serial.begin(115200);
  delay(100);  //short delay as next line wasn't showing up in Serial Monitor
  logWithTimestamp("MainController: Waking Up.");
  delay(100);  //short as display init is very fast and was clobbering above println
  resetBootPartition();
  displayController.init();
  httpController.init(&displayController);
  storageController.init(&displayController);

  pinMode(PURGE_CACHE_BUTTON, INPUT_PULLUP);
  if (digitalRead(PURGE_CACHE_BUTTON) == LOW) {
    logWithTimestamp("MainController: Purging cache because of button press.");
    storageController.purgeCache();
  }

  failCount = 0;
  logWithTimestamp("MainController: Setup complete");
}

void loop() {
  if (failCount >= 3) {
    logWithTimestamp("MainController: Too many failures.  Purging cache.");
    storageController.purgeCache();
    failCount = 0;
  }
  if (storageController.cacheHasImage()) {
    bool gotImage = storageController.getNextImage(image);
    if (gotImage) {
      logWithTimestamp("Displaying image.");
      displayController.displayImage(image);

      if (!storageController.cacheHasImage()) {  //repopulate cache if necessary
        populateCache();
      }

      // Check the reset reason
      esp_reset_reason_t resetReason = esp_reset_reason();
      if (resetReason == ESP_RST_DEEPSLEEP) {
        logWithTimestamp("MainController: Woke up from deep sleep, don't show message.");
      } else {
        logWithTimestamp("MainController: Wake up wasn't from deep sleep, show message.");
        displayNumberOfImagesInCache();
        delay(5000);
        displayController.dismissMessage();
      }

      goToDeepSleep();
    } else {
      failCount++;
      logWithTimestamp("MainController: Failed to get image.  Allowing another loop iteration.");
    }
  } else {
    populateCache();
  }
}