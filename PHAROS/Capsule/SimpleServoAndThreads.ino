#include <Wire.h>

#include <Adafruit_MCP9808.h>
#include <Adafruit_BNO08x.h>
#include "Adafruit_MPRLS.h"

#include <TeensyThreads.h>
#include <Servo.h>

#define SERVO_PIN 22
#define PRINT_DATA true

Servo pharosServo;
Adafruit_MCP9808 tempSensor = Adafruit_MCP9808();
Adafruit_MPRLS pressureSensor = Adafruit_MPRLS();
Adafruit_BNO08x imuSensor;

sh2_SensorValue_t sensorValue;
sh2_SensorId_t reportType = SH2_ARVR_STABILIZED_RV;
long reportIntervalUs = 5000;
int err; 

struct SensorData {
  float tempF;
  float pressure;
  float r, i, j, k;
};

SensorData sensorData;

void setReports(sh2_SensorId_t reportType, long reportInterval) {
  if (!imuSensor.enableReport(reportType, reportInterval)) {
    Serial.println("IMU ERROR - Set reports failed");
  }
}

void printReport() {
  Serial.print("Temp (F): "); Serial.println(sensorData.tempF);
  Serial.print("Pressure: "); Serial.println(sensorData.pressure);
  Serial.print("I: "); Serial.println(sensorData.i);
  Serial.print("J: "); Serial.println(sensorData.j);
  Serial.print("K: "); Serial.println(sensorData.k);
  Serial.print("R: "); Serial.println(sensorData.r);
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

void dataSend() {
  sensorData.tempF = tempSensor.readTempF();
  sensorData.pressure = pressureSensor.readPressure();

  if (imuSensor.getSensorEvent(&sensorValue)) {
    sensorData.r = sensorValue.un.arvrStabilizedRV.real;
    sensorData.i = sensorValue.un.arvrStabilizedRV.i;
    sensorData.j = sensorValue.un.arvrStabilizedRV.j;
    sensorData.k = sensorValue.un.arvrStabilizedRV.k;
  }

  // RFD
  // RockBlock

  if (PRINT_DATA) {
    printReport();
  }
}

void threadMoveCapsule() {
  while (true) {
    moveCapsule();
    threads.yield();
  }
}

void threadDataSend() {
  while (true) {
    dataSend();
    threads.delay(500);
  }
}

void setup() {
  pharosServo.attach(SERVO_PIN);

  if (!tempSensor.begin(0x19)) Serial.println("Failed to find Temperature Sensor");

  if (!pressureSensor.begin()) Serial.println("Failed to find Pressure Sensor");
  
  if (!imuSensor.begin_I2C()) Serial.println("Failed to find IMU");

  setReports(reportType, reportIntervalUs);

  threads.addThread(threadMoveCapsule);
  threads.addThread(threadDataSend);
}

void loop() {
}
