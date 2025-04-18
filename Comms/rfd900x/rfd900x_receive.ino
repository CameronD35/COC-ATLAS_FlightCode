#include <TimeLib.h>
#include <SD.h>
#include <SPI.h>

// SD card chip select pin
const int chipSelect = BUILTIN_SDCARD;

void setup() {
  Serial.begin(115200); // Debugging
  Serial1.begin(57600); // RFD900x communication

  // Initialize SD card
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    while (1);
  }
  Serial.println("SD card initialized successfully!");

  setTime(14, 0, 0, 26, 3, 2025);
}

void loop() {
  // Check if data is available from RFD900x
  Serial.println("Snail is listening\n");

  if (Serial1.available()) {
    String receivedData = Serial1.readStringUntil('\n'); // Read incoming data

    Serial.println("Snail received message!\n");

    String timeStamp = String(hour()) + ":" + String(minute()) + ":" + String(second()) + " " +
                       String(day()) + "/" + String(month()) + "/" + String(year());

    Serial.println("Received: " + receivedData); // Print to Serial Monitor

    String fullData = "{'timestamp': " + timeStamp + ", 'data': " + receivedData + " }";

    // Save data to SD card
    File dataFile = SD.open("data.txt", FILE_WRITE);
    if (dataFile) {
      dataFile.println(fullData);
      dataFile.close();
      // Print the saved data to the Serial Monitor
      Serial.println("Data received and saved: " + fullData);
    } else {
      Serial.println("Error opening file!");
    }
  }
  delay(1000);
}