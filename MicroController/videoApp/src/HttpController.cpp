#include "HttpController.h"
#include "Config.h"

#define BATTERY_PIN 35

void HttpController::init(DisplayController* displayController) {
  this->displayController = displayController;
}

void HttpController::connectWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    this->displayController->showMessage("Connecting to WiFi...");
    logWithTimestamp("HttpController: Connecting to WiFi...");
  }
  this->displayController->showMessage("Connected to WiFi");
  logWithTimestamp("HttpController: Connected to WiFi");
}

void HttpController::disconnectWiFi() {
  WiFi.disconnect();
  this->displayController->showMessage("Disconnected from WiFi");
  logWithTimestamp("HttpController: Disconnected from WiFi");
}

//A pointer is passed in of an array to be populated by this function.
void HttpController::fetchImage(uint8_t* image) {
  int payloadSize = imageBytes;
  HTTPClient http;

  float batteryVoltage = readBatteryVoltage();

  // Append the battery voltage as a query string parameter to the endpoint
  String endpointWithVoltageParam = String(httpEndpoint) + "&batteryVoltage=" + String(batteryVoltage, 2);

  http.begin(endpointWithVoltageParam);
  int httpCode = http.GET();

  if (httpCode > 0) {  // Check for the returning code
    if (httpCode == HTTP_CODE_OK) {
      WiFiClient stream = http.getStream();

      int contentLength = http.getSize();

      int headerLength = contentLength - payloadSize;

      // Create a dummy buffer
      uint8_t dummyBuffer[headerLength];

      // Read and discard the first headerLength bytes
      stream.readBytes(dummyBuffer, headerLength);

      // Now you can read the image data
      stream.readBytes(image, payloadSize);
      logWithTimestamp("HttpController: Successfully fetched image.");
    }
  } else {
    logWithTimestamp("HttpController: Error on HTTP request");
  }

  http.end();  // Free the resources
}

void HttpController::logWithTimestamp(const String& message) {
  // Get the number of milliseconds since the device started
  unsigned long currentTime = millis();

  // Convert milliseconds to seconds and format as [seconds.milliseconds]
  unsigned long seconds = currentTime / 1000;
  unsigned long milliseconds = currentTime % 1000;

  // Prepend the timestamp to the message and print it
  Serial.println("[" + String(seconds) + "." + String(milliseconds) + "] " + message);
}

float HttpController::readBatteryVoltage() {
  int raw = analogRead(BATTERY_PIN);
  float voltage = raw * (3.3 / 4095.0); // Convert ADC reading to voltage (assuming 3.3V reference).  ADC is 12 bit, max value is 4095 (starts at zero).
  voltage = voltage * 2; // assuming internal voltage divider halves the voltage
  voltage = voltage * 1.039; // Thom took the reading from the above line and measured the battery directly with a multimeter.  This factor roughly brought the two inline.
  logWithTimestamp("HttpController: Battery voltage is " + String(voltage, 2) + "V");
  return voltage;
}