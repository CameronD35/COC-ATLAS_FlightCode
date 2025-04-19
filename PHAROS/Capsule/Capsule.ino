#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BNO08x.h>
#include "Adafruit_MPRLS.h"

// Required for SPI but not I2C
#define BNO08x_RESET -1
#define MPRLS_RESET -1
#define MPRLS_EOC_PIN -1
Adafruit_MPRLS mpr = Adafruit_MPRLS(MPRLS_RESET, MPRLS_EOC_PIN);

struct euler_t {
  float yaw;
  float pitch;
  float roll;
} ypr;
static long last = 0;

Adafruit_BNO08x bno08x(BNO08x_RESET);
sh2_SensorValue_t sensorValue;
sh2_SensorId_t reportType = SH2_ARVR_STABILIZED_RV;
long reportIntervalUs = 5000; 

void setReports(sh2_SensorId_t reportType, long report_interval) {
  Serial.println("Setting desired reports");
  if (! bno08x.enableReport(reportType, report_interval)) {
    Serial.println("Could not enable stabilized remote vector");
  }
}

void setup() {
  // For the serial monitor
  Serial.begin(115200);
  // For the RFD
  Serial1.begin(57600);

  while (!Serial) delay(10);

  if (!bno08x.begin_I2C()){
    Serial.println("Failed to find BNO08x sensor.");
    while (1) { delay(10); }
  }

  Serial.println("Found IMU");

  setReports(reportType, reportIntervalUs);

  if (!mpr.begin()) {
    Serial.println("Failed to find MPRLS sensor.");
    while (1) { delay(10); }
  }

  Serial.println("Found Pressure sensor");

  delay(100);
  last = 0;
}

void quaternionToEuler(float qr, float qi, float qj, float qk, euler_t* ypr, bool degrees = false) {

    float sqr = sq(qr);
    float sqi = sq(qi);
    float sqj = sq(qj);
    float sqk = sq(qk);

    ypr->yaw = atan2(2.0 * (qi * qj + qk * qr), (sqi - sqj - sqk + sqr));
    ypr->pitch = asin(-2.0 * (qi * qk - qj * qr) / (sqi + sqj + sqk + sqr));
    ypr->roll = atan2(2.0 * (qj * qk + qi * qr), (-sqi - sqj + sqk + sqr));

    if (degrees) {
      ypr->yaw *= RAD_TO_DEG;
      ypr->pitch *= RAD_TO_DEG;
      ypr->roll *= RAD_TO_DEG;
    }
}

void quaternionToEulerRV(sh2_RotationVectorWAcc_t* rotational_vector, euler_t* ypr, bool degrees = false) {
    quaternionToEuler(rotational_vector->real, rotational_vector->i, rotational_vector->j, rotational_vector->k, ypr, degrees);
}


void loop() {
  float pressure_hPa = mpr.readPressure();
  Serial.println("Looping");
  
  if (bno08x.wasReset()){
    Serial.println("Sensor was reset");
  }

  if (bno08x.getSensorEvent(&sensorValue)) {

    quaternionToEulerRV(&sensorValue.un.arvrStabilizedRV, &ypr, true);
  

    
    // Think of a better way to measure time elapsed maybe?
    long now = micros();
    Serial.println("-----------------------------------");
    Serial.println("Time \t Yaw \t Pitch \t Roll \t Pressure (hPa)");
    Serial.print(now - last); Serial.print("\t"); 
    Serial.print(ypr.yaw); Serial.print("\t");
    Serial.print(ypr.pitch); Serial.print("\t");
    Serial.print(ypr.roll); Serial.print("\t");
    Serial.println(pressure_hPa);

    String dataPacket = String("{'time': ") + (now - last) + ", 'yaw': " + ypr.yaw + ", 'pitch': " + ypr.pitch + ", 'roll': " + ypr.roll + ", 'pressure': " + pressure_hPa + "}";

    last = now;

    // Send data over RFD900x
    Serial1.println(dataPacket);

    // Debugging
    Serial.println(dataPacket);
 }

  delay(1000);
}
