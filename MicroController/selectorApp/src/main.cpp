#include <Arduino.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"

const int switchPin = 15;  // GPIO15 for the switch

// Boot to a partition by subtype
bool bootToPartition(esp_partition_subtype_t subtype, const char* appName) {
  Serial.printf("Attempting to boot to %s\n", appName);
  
  // Find the partition by subtype
  const esp_partition_t* target_partition = esp_partition_find_first(
    ESP_PARTITION_TYPE_APP, subtype, NULL);
  
  if (target_partition != NULL) {
    Serial.printf("Found partition with subtype 0x%02x at address 0x%x\n", 
                 subtype, target_partition->address);
    
    // Set boot partition
    esp_err_t err = esp_ota_set_boot_partition(target_partition);
    if (err == ESP_OK) {
      Serial.printf("Boot partition set to %s, rebooting...\n", appName);
      delay(1000);
      ESP.restart();
      return true;
    } else {
      Serial.printf("Failed to set boot partition, error: %d\n", err);
      return false;
    }
  } else {
    Serial.printf("ERROR: Partition with subtype 0x%02x not found!\n", subtype);
    return false;
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("Selector app is running!");
  
  // List all available partitions
  Serial.println("Available partitions:");
  esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
  while (it != NULL) {
    const esp_partition_t* p = esp_partition_get(it);
    Serial.printf("  - '%s', type: %d, subtype: %d, address: 0x%x\n",
                 p->label, p->type, p->subtype, p->address);
    it = esp_partition_next(it);
  }
  esp_partition_iterator_release(it);
  
  // Configure GPIO15 as input with pull-up resistor
  pinMode(switchPin, INPUT_PULLUP);
  
  // Read the switch state
  int switchState = digitalRead(switchPin);
  
  // Boot to the appropriate partition based on switch position
  if (switchState == LOW) {
    Serial.println("Switch is ON - Booting to Weather App");
    bootToPartition(ESP_PARTITION_SUBTYPE_APP_OTA_0, "Weather App");
  } else {
    Serial.println("Switch is OFF - Booting to Video App");
    bootToPartition(ESP_PARTITION_SUBTYPE_APP_OTA_1, "Video App");
  }
  
  // If we get here, boot failed
  Serial.println("ERROR: Boot failed!");
}

void loop() {
  // The loop function is empty because we already handled the boot logic in setup()
}
