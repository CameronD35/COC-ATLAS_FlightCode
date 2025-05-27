#include <Wire.h>
#include <Adafruit_MCP9808.h>
#include <Adafruit_BNO08x.h>
#include "Adafruit_MPRLS.h"
#include <IridiumSBD.h>
#include <ArduinoJson.h>

// MsgPack code from Owen! Thank you!!
// SOURCE: https://github.com/MoonstoneStudios/MsgPackTestRSX/tree/main
#include <MsgPack.h>

// These aren't the final pins
// Final will have RFSerial as Serial1
//            and IridiumSerial as Serial2
// Keeping these here to print and debug
// #define RFSerial Serial2 
#define IridiumSerial Serial3

Adafruit_MCP9808 tempSensor = Adafruit_MCP9808();
Adafruit_MPRLS mpr = Adafruit_MPRLS();
Adafruit_BNO08x bno08x;
IridiumSBD rockblock(IridiumSerial);
StaticJsonDocument<200> json;


sh2_SensorValue_t sensorValue;
sh2_SensorId_t reportType = SH2_ARVR_STABILIZED_RV;
long reportIntervalUs = 5000;
int signalQuality = -1;
int err;

void setReports(sh2_SensorId_t reportType, long report_interval){
    if (! bno08x.enableReport(reportType, report_interval)) {
        Serial.println("Could not enable stabilized remote vector");
    }
}

void setup() {
    Serial.begin(9600);
    // RFSerial.begin(57600);
    IridiumSerial.begin(19200);
    
    Serial.println("Starting RockBlock...");
    err = rockblock.begin();

    if (err != ISBD_SUCCESS) {
      Serial.print("RockBlock failed: ");
      Serial.println(err);

      if (err == ISBD_NO_MODEM_DETECTED) {
        Serial.println("Could not find RockBlock; check wiring or make sure flow control is off");
      }

      return;
    }

    if (!tempSensor.begin(0x19)) {
        Serial.println("Failed to find Temperature Sensor");
    }
    Serial.println("Found Temp sensor");
    
    if (! mpr.begin()) {
        Serial.println("Failed to find Pressure Sensor");
    }
    Serial.println("Found Pressure sensor");

    if (!bno08x.begin_I2C()){
        Serial.println("Failed to find IMU");
    }
    Serial.println("Found IMU");

    setReports(reportType, reportIntervalUs);

    delay(1000);
}

void loop() {
    json.clear();

    if (bno08x.wasReset()) {
        Serial.println("Sensor was reset");
        setReports(reportType, reportIntervalUs);
    }

    err = rockblock.getSignalQuality(signalQuality);

    if (bno08x.getSensorEvent(&sensorValue)){
        json["t"] = tempSensor.readTempF();
        json["p"] = mpr.readPressure();
        json["r"] = sensorValue.un.arvrStabilizedRV.real;
        json["i"] = sensorValue.un.arvrStabilizedRV.i;
        json["j"] = sensorValue.un.arvrStabilizedRV.j;
        json["k"] = sensorValue.un.arvrStabilizedRV.k;
        json["s"] = signalQuality;

    String jsonStr;
    int bytes = serializeJson(json, jsonStr);

    Serial.println("\n----------------------");
    Serial.print("JSON: ");
    Serial.println(jsonStr);
    Serial.print("BYTES: ");
    Serial.println(bytes);

    // DUCT TAPE FIX:
    // MsgPack continues to fill buffer indefinitely and
    // buffer().clear() is private
    //
    // I was too lazy to read documentation; will fix later
    MsgPack::Packer packer;
    packer.serialize(json);
    
    const uint8_t data = packer.data();
    size_t dataSize = packer.size();

    // NOTE: sendSBDBinary handles flush; no need to manually flush
    int status = rockblock.sendSBDBinary(data, dataSize);
    // RFSerial.write(data, dataSize);
    // RFSerial.flush();

    if (status == ISBD_SUCCESS) {
      Serial.println("Sent data over RockBlock successfully");
    } else {
      Serial.print("RockBlock failed: ");
      Serial.println(status);
    }

    Serial.print("\n\nMSGPACK: ");
    Serial.print("BYTES: ");
    Serial.println(dataSize);
    Serial.print("DATA: ");

    for (unsigned int i = 0; i < packer.size(); i++) {
      uint8_t hex = packer.data()[i];
      Serial.print(hex, HEX);
    }
    }   
    delay(500);
}