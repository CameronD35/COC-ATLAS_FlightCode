#include <SD.h>
#include <MsgPack.h>
#include <ArduinoJson.h>

#define RFSerial Serial1 
#define DATA_BUFFER_SIZE 256

uint8_t msgpackBuffer[DATA_BUFFER_SIZE];
size_t bufferIndex = 0;
uint16_t expectedLen = 0;

// Used for sanity check later on; ensures that the transmitter and receiver are not out of sync
// and sending messages that are cutting off in the wrong places
unsigned long lastByteTime = 0;
const unsigned long MSG_TIMEOUT_MS = 500;

// writeToSD() : Takes in the received message in JSON format and appends it to the pharosData.jsonl file
void writeToSD(const StaticJsonDocument<256>& doc) {
  File logFile = SD.open("pharosData.jsonl", FILE_WRITE);

  if (logFile) {
    serializeJson(doc, logFile);
    logFile.println();
    logFile.flush();
    logFile.close();
  } else {
    Serial.println("Failed to open log file");
  }
}

void setup() {
  Serial.begin(9600);
  RFSerial.begin(57600);
  
  Serial.println("Snail is waking up!");

  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("Failed to initialize SD card!");
    while(1);
  }
  Serial.println("Connected to SD card!");

  delay(100);
}

void loop() {
  // Uses length prefix to prevent partial data receiving 
  if (expectedLen == 0 && RFSerial.available() >= 2) {
    expectedLen = RFSerial.read();
    expectedLen |= (RFSerial.read() << 8);
    bufferIndex = 0;
    lastByteTime = millis();
  }

  // Prevents overflow
  if (expectedLen > DATA_BUFFER_SIZE) {
    Serial.println("Message exceeds buffer size; resetting");
    expectedLen = 0;
    bufferIndex = 0;
    return;
  }

  // Reads in data if there's a message available
  while (expectedLen > 0 && RFSerial.available() && bufferIndex < expectedLen && bufferIndex < DATA_BUFFER_SIZE) {
    msgpackBuffer[bufferIndex++] = RFSerial.read();
    lastByteTime = millis();
  }

  // If there is a message and it' sin the buffer, we deserialize it, display it, and write it to the SD card
  if (expectedLen > 0 && bufferIndex == expectedLen) {
    StaticJsonDocument<256> json;
    DeserializationError err = deserializeMsgPack(json, msgpackBuffer, bufferIndex);

    if (!err) {
      float tempF = json["t"];
      float pressHPa = json["p"];
      float imuR = json["r"];
      float imuI = json["i"];
      float imuJ = json["j"];
      float imuK = json["k"];
      float rockblockSignalQuality = json["s"];
      uint32_t timeSent = json["ts"];


      // Printing for debugging reasons; feel free to delete
      Serial.print("Temperature (F): "); Serial.println(tempF);
      Serial.print("Pressure (hPa): "); Serial.println(pressHPa);
      Serial.print("IMU: [ "); Serial.print(imuR); Serial.print(", "); Serial.print(imuI); Serial.print(", "); Serial.print(imuJ); Serial.print(", "); Serial.print(imuK); Serial.println(" ]");
      Serial.print("SignalQuality: "); Serial.println(rockblockSignalQuality);
      Serial.print("Time sent: "); Serial.println(timeSent);

      Serial.println("-----------------------------------------");

      serializeJsonPretty(json, Serial);
      Serial.println();


      writeToSD(json);
    } else {
      // If there is something in the buffer but it's the wrong length, we look at it to see what went wrong
      // This is just a debugging thing; feel free to erase.
      Serial.print("Deserialization error: "); Serial.println(err.c_str());
      Serial.print("Raw buffer: ");

      for (size_t i =0; i < bufferIndex; i++) {
        Serial.print(msgpackBuffer[i], HEX); Serial.print(" ");
      }
      
      Serial.println();
    }

    // Resets the buffer for the next message.
    expectedLen = 0;
    bufferIndex = 0;
  }

  /*
  SANITY CHECK - If the transmit and receive are out of sync, the receiver may start and end
  in the wrong place. This code double-checks to see if it's been receiving data for a long time
  without finding an end; if this is the case, it drops everything and restarts.
  */
  if (expectedLen > 0 && (millis() - lastByteTime > MSG_TIMEOUT_MS)) {
    Serial.println("Message receive timeout/reset!");
    expectedLen = 0;
    bufferIndex = 0;
  }

  // Set to check for messages at a much quicker speed than the capsule is sent to send them
  delay(10);
}