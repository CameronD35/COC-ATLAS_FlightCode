#include <SD.h>
#include <ArduinoJson.h>

#define RFSerial Serial1 

String inputBuffer;

void writeToSD(const JsonDocument& doc) {
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
  while (RFSerial.available()) {
    char c = RFSerial.read();
    if (c == '\n') {
      inputBuffer.trim();

      if (inputBuffer.length() > 0) {
        StaticJsonDocument<256> json;
        DeserializationError err = deserializeJson(json, inputBuffer);

        if (!err) {
          float tempF = json["t"];
          float pressHPa = json["p"];
          float imuR = json["r"];
          float imuI = json["i"];
          float imuJ = json["j"];
          float imuK = json["k"];
          float rockblockSignalQuality = json["s"];

          Serial.print("Temperature (F): "); Serial.println(tempF);
          Serial.print("Pressure (hPa): "); Serial.println(pressHPa);
          Serial.print("IMU: ["); Serial.print(imuR); Serial.print(imuI); Serial.print(imuJ); Serial.print(imuK);
          Serial.print("SignalQuality: "); Serial.print(rockblockSignalQuality);

          Serial.println("-----------------------------------------");

          serializeJsonPretty(json, Serial);
          Serial.println();
          writeToSD(json);
        } else {
          Serial.print("Deserialization error: "); Serial.println(err.c_str());
          Serial.print("Raw buffer: "); Serial.println(inputBuffer);
        }
      }
      inputBuffer = "";
  } else {
    inputBuffer += c;
    if (inputBuffer.length() > 256) inputBuffer = "";
  }
}
  
  delay(10);
}