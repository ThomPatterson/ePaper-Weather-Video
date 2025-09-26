#include "DisplayController.h"
#include "fonts/FreeMonoBold9pt7b.h"
#include "Config.h"


// Create an instance of the display
#define GxEPD2_DRIVER_CLASS GxEPD2_750_GDEY075T7
GxEPD2_BW< GxEPD2_DRIVER_CLASS, GxEPD2_DRIVER_CLASS::HEIGHT / 2 > display(GxEPD2_DRIVER_CLASS(cs_pin, dc_pin, rst_pin, busy_pin));


void DisplayController::init() {
  display.init(115200);
  logWithTimestamp("DisplayController: Display initialized");

  display.setRotation(2); //ribbon at top
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
}


/*
  Painting the image as a partial window update looks cleaner. The image pops in directly from a white screen.
  Doing it as a full window update results in an inverted image appearing for a fraction of a second.
  Unfortunately the library doesn't have a rotate option, and the drawBitmap function seems to ignore display.setRotation,
  so I am rotating the image myself.
  I am drawing the bitmap inverted as the alternative is to paint the entire screen black and then draw white pixels.
*/
void DisplayController::displayImage(const uint8_t* image) {
  //store a reference to this image
  lastImage = image;

  // Rotate the image
  uint8_t* rotatedImage = rotateImage180(image, 800, 480);

  display.clearScreen(GxEPD_WHITE);  //clear the screen to eliminate ghosting


  // Display the rotated image
  display.setPartialWindow(0, 0, 800, 480);
  display.firstPage();
  do {
    display.drawInvertedBitmap(0, 0, rotatedImage, 800, 480, GxEPD_BLACK);
  } while (display.nextPage());

  delete[] rotatedImage;  //free memory
}

/**
 * This function rotates a given bitmap image by 180 degrees.
 *
 * @param image: A pointer to the original bitmap image data. The image is assumed to be a monochrome bitmap (1 bit per pixel).
 * @param width: The width of the image in pixels.
 * @param height: The height of the image in pixels.
 *
 * @return: A pointer to the new bitmap image data after rotation.
 *
 * The function works by creating a new bitmap array of the same size as the original. It then iterates over each pixel in the original image.
 * For each pixel, it calculates the corresponding position in the rotated image by subtracting the current y-coordinate from the height of the image (to flip the image vertically), and keeping the x-coordinate the same.
 * It then reads the pixel value from the original image and writes it to the corresponding position in the rotated image.
 * The pixel value is read using bitwise operations to extract the correct bit from the byte in the original image data.
 * The pixel value is written to the rotated image using bitwise operations to set the correct bit in the byte in the rotated image data.
 * If the pixel in the original image is set (i.e., the pixel value is 1), the corresponding bit in the rotated image is set. If the pixel in the original image is not set (i.e., the pixel value is 0), the corresponding bit in the rotated image is cleared.
 * Finally, the function returns a pointer to the rotated image data.
 */
uint8_t* DisplayController::rotateImage180(const uint8_t* image, uint16_t width, uint16_t height) {
  uint8_t* rotatedImage = new uint8_t[width * height / 8];  // Assumes 1 bit per pixel

  for (uint16_t y = 0; y < height; y++) {
    for (uint16_t x = 0; x < width; x++) {
      // Get the pixel value
      uint8_t pixel = image[(height - y - 1) * width / 8 + x / 8] & (0x80 >> (x % 8));
      // Set the pixel in the rotated image
      if (pixel) {
        rotatedImage[y * width / 8 + x / 8] |= (0x80 >> (x % 8));
      } else {
        rotatedImage[y * width / 8 + x / 8] &= ~(0x80 >> (x % 8));
      }
    }
  }

  return rotatedImage;
}



/*
  Define message dimensions
*/
const int lineHeight = 20;
int16_t msg_w = 350;
int16_t msg_h = 50;
int16_t msg_x = 0;
int16_t msg_y = 480 - msg_h;

/*
  Draws a white rectangle in the bottom left of the screen
  and shows the message
*/
void DisplayController::showMessage(const char* message) {

  // Set the partial window
  display.setPartialWindow(msg_x, msg_y, msg_w, msg_h);

  // Start a new page for drawing
  display.firstPage();
  do {
    // Draw a white rectangle in the partial window
    display.fillRect(msg_x, msg_y, msg_w, msg_h, GxEPD_WHITE);

    // Set the cursor to the start of the rectangle, adjusted for line height
    display.setCursor(msg_x, msg_y + lineHeight);

    // Print the message
    display.print(message);
  } while (display.nextPage());  // Update the display buffer and write it to the display
}

/*
  Do a partial window update by redrawing the image (which we keep a reference to as "lastImage")
  in the same area that the message used.
  Rather than using the rotated image and trying to draw a partial bitmap I'm just drawing the pixels I need.
  Adding 10px to message to fix bug where black line was being left behind.  I suspect this method needs a multiple of 8.
*/
void DisplayController::dismissMessage() {

  // Set the partial window
  display.setPartialWindow(msg_x, msg_y, msg_w+10, msg_h);

  // Redraw the part of the image that was covered by the message
  display.firstPage();
  do {
    display.fillScreen(GxEPD_BLACK);
    for (uint16_t i = 0; i < msg_h; i++) {
      for (uint16_t j = 0; j < msg_w+10; j++) {
        // Calculate the index in the image data
        uint32_t idx = ((480 - (msg_y + i) - 1) * 800 + (msg_x + j)) / 8;  // Assuming the image width is 800 pixels

        // Get the pixel value from the last displayed image
        uint8_t pixel = lastImage[idx] & (0x80 >> ((msg_x + j) % 8));

        // Draw the pixel
        display.drawPixel(msg_x + j, msg_y + i, pixel ? GxEPD_WHITE : GxEPD_BLACK);
      }
    }
  } while (display.nextPage());
}

void DisplayController::powerDown() {
  display.powerOff();
}

void DisplayController::logWithTimestamp(const String& message) {
  // Get the number of milliseconds since the device started
  unsigned long currentTime = millis();

  // Convert milliseconds to seconds and format as [seconds.milliseconds]
  unsigned long seconds = currentTime / 1000;
  unsigned long milliseconds = currentTime % 1000;

  // Prepend the timestamp to the message and print it
  Serial.println("[" + String(seconds) + "." + String(milliseconds) + "] " + message);
}