#include <ArduinoJson.h>

#define RFSerial Serial1 

void setup() {
  Serial.begin(9600);
  RFSerial.begin(57600);
  
  Serial.println("Snail is waking up!");

  delay(100);
}

void loop() {
  if (RFSerial.available()) {
    StaticJsonDocument<200> doc;

    DeserializationError err = deserializeMsgPack(doc, RFSerial);

    if (!err) {
      float tempF = doc["t"];
      float presHPa = doc["p"];
      float imuR = doc["r"];
      float imuI = doc["i"];
      float imuJ = doc["j"];
      float imuK = doc["k"];
      float rockblockSignalQuality = doc["s"];

      Serial.print("Temperature (F): "); Serial.println(tempF);
      Serial.print("Pressure (hPa): "); Serial.println(pressHPa);
      Serial.print("IMU: ["); Serial.print(imuR); Serial.print(imuI); Serial.print(imuJ); Serial.println(imuK);
      Serial.print("SignalQuality: "); Serial.print(rockblockSignalQuality);
    }   
  }
  
  delay(250);
}