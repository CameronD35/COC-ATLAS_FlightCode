#include <Wire.h>
#include <Adafruit_MCP9808.h>
#include <Adafruit_BNO08x.h>
#include "Adafruit_MPRLS.h"

// Thank you for your recommendation, Owen!
#define RFSerial Serial2 

Adafruit_MCP9808 tempSensor = Adafruit_MCP9808();
Adafruit_MPRLS mpr = Adafruit_MPRLS();
Adafruit_BNO08x bno08x;

sh2_SensorValue_t sensorValue;
sh2_SensorId_t reportType = SH2_ARVR_STABILIZED_RV;
long reportIntervalUs = 5000;

void setReports(sh2_SensorId_t reportType, long report_interval){
    if (! bno08x.enableReport(reportType, report_interval)) {
        Serial.println("Could not enable stabilized remote vector");
    }
}

struct euler_t {
  float yaw;
  float pitch;
  float roll;
} ypr;

struct SensorData {
    float tempF;
    float pressure_hPa;
    float yaw;
    float pitch;
    float roll;
};

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

void setup() {
    Serial.begin(9600);
    RFSerial.begin(57600);
    
    if (!tempSensor.begin(0x19)) {
        Serial.println("Failed to find Temperature Sensor");
        //while (1);
    }
    Serial.println("Found Temp sensor");
    
    if (! mpr.begin()) {
        Serial.println("Failed to find Pressure Sensor");
        while (1) {
            delay(10);
        }
    }
    Serial.println("Found Pressure sensor");

    if (!bno08x.begin_I2C()){
        Serial.println("Failed to find IMU");
    }
    Serial.println("Found IMU");

    setReports(reportType, reportIntervalUs);

    delay(100);
}

void loop() {
    if (bno08x.wasReset()) {
        Serial.println("Sensor was reset");
        setReports(reportType, reportIntervalUs);
    }

    if (bno08x.getSensorEvent(&sensorValue)){
        quaternionToEulerRV(&sensorValue.un.arvrStabilizedRV, &ypr, true);
    }

    SensorData data = { tempSensor.readTempF(), mpr.readPressure(), ypr.yaw, ypr.pitch, ypr.roll};
    RFSerial.write((uint8_t*)&data, sizeof(data));
    // RFSerial.write("HULLO");

    Serial.println("----------------------");
    Serial.print("Temperature: ");
    Serial.print(data.tempF);
    Serial.println(" °F");
    Serial.print("Pressure: ");
    Serial.println(data.pressure_hPa);
    Serial.print(ypr.yaw);                Serial.print("\t");
    Serial.print(ypr.pitch);              Serial.print("\t");
    Serial.println(ypr.roll);
    
    delay(750);
}