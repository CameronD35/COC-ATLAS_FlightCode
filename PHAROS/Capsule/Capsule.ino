#include <Wire.h>

#include <Adafruit_MCP9808.h>
#include <Adafruit_BNO08x.h>
#include "Adafruit_MPRLS.h"

#include <TeensyThreads.h>
#include <ArduinoJson.h>
#include <MsgPack.h>
#include <Servo.h>
#include <IridiumSBD.h>
#define DATA_BUFFER_SIZE 256

#define SERVO_PIN 22
#define RFSerial Serial1
#define IridiumSerial Serial2

bool PRINT_DATA = true;
bool USE_RFD = true;
bool USE_ROCKBLOCK = true;
bool USE_SERVO = true;

Servo pharosServo;
Adafruit_MCP9808 tempSensor = Adafruit_MCP9808();
Adafruit_MPRLS pressureSensor = Adafruit_MPRLS();
Adafruit_BNO08x imuSensor;
IridiumSBD rockblock(IridiumSerial);

sh2_SensorValue_t sensorValue;
sh2_SensorId_t reportType = SH2_ARVR_STABILIZED_RV;
long reportIntervalUs = 5000;
int signalQuality = -1;
int err; 

volatile uint8_t sharedData[DATA_BUFFER_SIZE];
volatile size_t sharedDataLen = 0;
Threads::Mutex dataMutex;
volatile bool newDataAvailable = false;


void setReports(sh2_SensorId_t reportType, long reportInterval) {
  if (!imuSensor.enableReport(reportType, reportInterval)) {
    Serial.println("IMU ERROR - Set reports failed");
  }
}

void threadSensorAndRFD() {
  while (true) {
    if (imuSensor.getSensorEvent(&sensorValue)) {
      
      MsgPack::map_t<const char*, float> sensorData = {
        { "t", tempSensor.readTempF() },
        { "p", pressureSensor.readPressure() },
        { "r", sensorValue.un.arvrStabilizedRV.real },
        { "i", sensorValue.un.arvrStabilizedRV.i },
        { "j", sensorValue.un.arvrStabilizedRV.j },
        { "k", sensorValue.un.arvrStabilizedRV.k },
        { "s", (float)signalQuality },
        { "ts", (uint32_t)millis() }
      };
      
      MsgPack::Packer packer;
      packer.serialize(sensorData);
      const uint8_t* msgpackBuffer = packer.data();
      size_t len = packer.size();

      if (USE_RFD) {
        uint16_t protocol_len = packer.size();
        RFSerial.write((uint8_t*)&len, 2);
        RFSerial.write(msgpackBuffer, len);
      }

      dataMutex.lock();
      memcpy((void*)sharedData, msgpackBuffer, len);
      sharedDataLen = len;
      newDataAvailable = true;
      dataMutex.unlock();

      if (PRINT_DATA) {
        Serial.print("MsgPack:");

        for (size_t i = 0; i < len; i++) {
          Serial.print(msgpackBuffer[i], HEX); Serial.print(" ");
        }

        Serial.println();
      }
    }
    
    threads.delay(500);
  }
}

void threadRockblock() {
  // Once again, this is a thread
  while (true) {
    bool sendNow = false;
    size_t len;
    uint8_t buffer[DATA_BUFFER_SIZE];

    dataMutex.lock();

    if (newDataAvailable) {
      len = sharedDataLen;
      memcpy(buffer, (const void*)sharedData, len);
      newDataAvailable = false;
      sendNow = true;
    }

    dataMutex.unlock();

    if (sendNow) {
      signalQuality = -1;
      err = rockblock.getSignalQuality(signalQuality);

      // Rockblock check signal quality - only send when 1 or higher
      if (signalQuality > 1 && err == ISBD_SUCCESS) {
        Serial.println("RockBlock signal greater than 1; trying to send; might take a sec.");
        err = rockblock.sendSBDBinary((const uint8_t*)buffer, len);

        // Rockblock response send conditional-----------------
        if (err == ISBD_SUCCESS) {
          if (PRINT_DATA) Serial.println("Message sent! Check AWS!");
        } else {
          if (PRINT_DATA) Serial.print("Unable to send message: "); Serial.println(err);
        } // End response send conditional---------------------
      } else {
        if (PRINT_DATA) Serial.println("RockBlock: No signal; Point patch antenna toward sky for better reception");
      } // End signal quality check
    } // End check for whether the signal should be sent
    threads.delay(500); // Check every second; Rockblock takes a while to send
  }
}

void moveCapsule(){
  for (int i = 0; i <= 180; i++) {
    pharosServo.write(i);
    delay(15);
  }

  for (int i = 180; i >= 0; i--) {
    pharosServo.write(i);
    delay(15);
  }
}

void threadMoveCapsule() {
  // Final thread
  while (true) {
    moveCapsule();
    threads.yield();
  }
}

void setup() {
  if (PRINT_DATA) {
    Serial.begin(9600);
    Serial.print("Initializing serials, servo, and sensors");
  }

  if (USE_RFD) {
    RFSerial.begin(57600);
  }

  if (USE_ROCKBLOCK) {
    IridiumSerial.begin(19200);
    err = rockblock.begin();

    if (err != ISBD_SUCCESS) {
      if (PRINT_DATA) {
        Serial.print("Rockblock failed: "); Serial.println(err);
      }

      USE_ROCKBLOCK = false;
    }
  }

  if (USE_SERVO) {
    pharosServo.attach(SERVO_PIN);
    // pharosServo.write();
  }
  
  if (!tempSensor.begin(0x19) && PRINT_DATA) Serial.println("Failed to find Temperature Sensor");

  if (!pressureSensor.begin() && PRINT_DATA) {
    Serial.println("Failed to find Pressure Sensor");
  } else {
    Serial.println("Did find pressure sensor; that's not the problem.");
  }
  
  if (!imuSensor.begin_I2C() && PRINT_DATA) Serial.println("Failed to find IMU");

  setReports(reportType, reportIntervalUs);

  threads.addThread(threadMoveCapsule);
  threads.addThread(threadSensorAndRFD);
  threads.addThread(threadRockblock);
}

void loop() {
}
