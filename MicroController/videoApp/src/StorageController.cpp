#include "StorageController.h"
#include "Config.h"

int cacheFileNumber = -1;

#define FORMAT_ON_FAIL true
#define BASE_PATH      "/videoFrames" //cannot be "/"
#define MAX_OPEN_FILES 5
#define PARTITION_LABEL "littlefs"


void StorageController::init(DisplayController* displayController) {
  this->displayController = displayController;

  /*
    Parameters of LittleFS.begin

    Parameter	      Type	  Default	    Description
    formatOnFail	  bool	  false	      If true, will format filesystem if mounting fails
    basePath	      const   char*	      "/littlefs"	Mount path in the VFS
    maxOpenFiles	  uint8_t	5	Max       simultaneous open files
    partitionLabel	const   char*	NULL	Partition label to mount (e.g., "littlefs" — must match partition table)
  */
  if (!LittleFS.begin(FORMAT_ON_FAIL, BASE_PATH, MAX_OPEN_FILES, PARTITION_LABEL)) {
    logWithTimestamp("Mount failed — attempting to format...");
    
    if (LittleFS.format()) {
      logWithTimestamp("Format succeeded — re-mounting...");
      if (LittleFS.begin(FORMAT_ON_FAIL, BASE_PATH, MAX_OPEN_FILES, PARTITION_LABEL)) {
        logWithTimestamp("Mount after format succeeded!");
      } else {
        logWithTimestamp("Mount failed even after format — check partition config.");
        while (1);  // Halt
      }
    } else {
      logWithTimestamp("Format failed — check partition label/size.");
      while (1);  // Halt
    }
  } else {
    logWithTimestamp("LittleFS mounted successfully!");
  }

  //showAvailableSpace();
}

void StorageController::purgeCache() {
  this->displayController->showMessage("Formatting LittleFS");
  LittleFS.format();
  cacheFileNumber = -1;
}

void StorageController::showAvailableSpace() {
  size_t totalBytes = LittleFS.totalBytes();
  size_t usedBytes = LittleFS.usedBytes();
  size_t availableBytes = totalBytes - usedBytes;

  //TODO: look into combining this into a single multi-line string
  this->displayController->showMessage(("Total LittleFS space: " + std::to_string(totalBytes / 1024) + " KB").c_str());
  this->displayController->showMessage(("Used LittleFS space: " + std::to_string(usedBytes / 1024) + " KB").c_str());
  this->displayController->showMessage(("Available LittleFS space: " + std::to_string(availableBytes / 1024) + " KB").c_str());
}

bool StorageController::cacheHasImage() {
  File root = LittleFS.open("/");
  if (!root) {
    return false;
  }

  if (!root.isDirectory()) {
    return false;
  }

  File file = root.openNextFile();
  bool hasImage = (file != NULL);

  file.close();
  root.close();

  if (!hasImage) {
    cacheFileNumber = -1;  //reset the file number to 0 when the cache runs dry
  }

  return hasImage;
}

/*
  Beware: this function takes about 4 seconds to execute
*/
int StorageController::getLargestFileNumber() {
  File root = LittleFS.open("/");
  if (!root) {
    return -1;
  }

  if (!root.isDirectory()) {
    return -1;
  }

  int largestNumber = -1;
  File file = root.openNextFile();
  while (file) {
    std::string filename = file.name();
    //logWithTimestamp(filename.c_str());
    int start = filename.find("image") + 5;
    int end = filename.find(".bmp");
    int number = std::stoi(filename.substr(start, end - start));
    if (number > largestNumber) {
      largestNumber = number;
    }
    file = root.openNextFile();
  }

  //logWithTimestamp("Largest number is ");
  //logWithTimestamp(largestNumber);

  file.close();
  root.close();
  return largestNumber;
}

/*
  Beware: this function takes about 4 seconds to execute
*/
int StorageController::getSmallestFileNumber() {
  File root = LittleFS.open("/");
  if (!root) {
    return -1;
  }

  if (!root.isDirectory()) {
    return -1;
  }

  int smallestNumber = INT_MAX;
  File file = root.openNextFile();
  while (file) {
    std::string filename = file.name();
    int start = filename.find("image") + 5;
    int end = filename.find(".bmp");
    int number = std::stoi(filename.substr(start, end - start));
    if (number < smallestNumber) {
      smallestNumber = number;
    }
    file = root.openNextFile();
  }

  file.close();
  root.close();
  return (smallestNumber == INT_MAX) ? -1 : smallestNumber;
}

//return a count of how many files are cached
int StorageController::getCacheSize() {
  File root = LittleFS.open("/");
  if (!root) {
    return -1;
  }

  if (!root.isDirectory()) {
    return -1;
  }

  int fileCount = 0;
  File file = root.openNextFile();
  while (file) {
    fileCount++;
    file = root.openNextFile();
  }

  file.close();
  root.close();
  return fileCount;
}

bool StorageController::cacheHasRoomForAnotherImage() {
  size_t totalBytes = LittleFS.totalBytes();
  size_t usedBytes = LittleFS.usedBytes();
  size_t availableBytes = totalBytes - usedBytes;
  logWithTimestamp(String("StorageController: Available bytes of storage: ") + String(availableBytes));
  return (availableBytes > 49000);  //leave a little headroom
}

void StorageController::writeImageToCache(uint8_t* image) {
  if (cacheFileNumber == -1) {
    cacheFileNumber = getLargestFileNumber() + 1;
  }

  // Generate a unique filename
  std::string filename = "/image" + zeroPad(cacheFileNumber++, 4) + ".bmp";

  // Open the file in write mode
  File file = LittleFS.open(filename.c_str(), FILE_WRITE);

  if (!file) {
    this->displayController->showMessage("Failed to open file for writing");
    logWithTimestamp(("StorageController: Failed to open file " + filename + " for writing.  Purging cache.").c_str());
    purgeCache();  //something is likely corrupt in flash memory, blow it all away
    return;
  }

  // Write the image data to the file and check the number of bytes written
  size_t bytesWritten = file.write(image, imageBytes);
  file.close();  // Close the file

  if (bytesWritten < imageBytes) {
    this->displayController->showMessage("Failed to write image!");
    logWithTimestamp(("StorageController: Failed to write entire image to file " + filename + ". Bytes written: " + String(bytesWritten).c_str()).c_str());
    logWithTimestamp("Assuming filesystem corruption.  Purging cache.");
    purgeCache();  //something is likely corrupt in flash memory, blow it all away
  } else {
    this->displayController->showMessage(("Image written to " + filename).c_str());
    logWithTimestamp(("StorageController: Successfully wrote image to file " + filename).c_str());
  }
}

bool StorageController::getNextImage(uint8_t* image) {

  int lowestFileNumber = this->getSmallestFileNumber();
  if (lowestFileNumber == -1) {
    logWithTimestamp("StorageController: Unable to determine lowest file number.");
    return false;
  }

  // Generate a unique filename
  std::string lowestFilename = "/image" + zeroPad(lowestFileNumber, 4) + ".bmp";

  if (!lowestFilename.empty()) {

    logWithTimestamp(String("StorageController: Reading ") + lowestFilename.c_str());

    // Double check if the file exists
    if (!LittleFS.exists(lowestFilename.c_str())) {
      logWithTimestamp(String("StorageController: This shouldn't be possible.  File does not exist: ") + lowestFilename.c_str());
      return false;
    }

    // Open the file
    File imageFile = LittleFS.open(lowestFilename.c_str());
    if (!imageFile) {
      logWithTimestamp("StorageController: Failed to open image file.  Deleting it.");
      LittleFS.remove(lowestFilename.c_str());  // Delete the file
      return false;
    }

    // Check the file size
    if (imageFile.size() == 0) {
      logWithTimestamp("StorageController: File is empty.  Deleting it.");
      LittleFS.remove(lowestFilename.c_str());  // Delete the file
      return false;
    }

    // Read the file
    size_t bytesRead = imageFile.read(image, imageBytes);
    if (bytesRead == 0) {
      logWithTimestamp("StorageController: Failed to read image file.  Deleting it.");
      LittleFS.remove(lowestFilename.c_str());  // Delete the file
      return false;
    } else if (bytesRead == -1) {
      logWithTimestamp("StorageController: Error occurred while reading file.  Deleting it.");
      LittleFS.remove(lowestFilename.c_str());  // Delete the file
      return false;
    }

    imageFile.close();                      //free resources
    LittleFS.remove(lowestFilename.c_str());  // Delete the file.  Think of this as popping off the queue.
  } else {
    logWithTimestamp("StorageController: This shouldn't be possible.  No file was found");
    return false;
  }

  return true;
}

std::string StorageController::zeroPad(int num, int size) {
  std::string s = std::to_string(num);
  while (s.size() < size) s = "0" + s;
  return s;
}

void StorageController::logWithTimestamp(const String& message) {
  // Get the number of milliseconds since the device started
  unsigned long currentTime = millis();

  // Convert milliseconds to seconds and format as [seconds.milliseconds]
  unsigned long seconds = currentTime / 1000;
  unsigned long milliseconds = currentTime % 1000;

  // Prepend the timestamp to the message and print it
  Serial.println("[" + String(seconds) + "." + String(milliseconds) + "] " + message);
}