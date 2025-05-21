#include <Adafruit_BNO08x.h>
#include <sh2.h>
#include <sh2_SensorValue.h>
#include <sh2_err.h>
#include <sh2_hal.h>
#include <sh2_util.h>
#include <shtp.h>
#include <Servo.h>
#include <math.h>

#define BNO08X_CS 10
#define BNO08X_INT 9
#define BNO08X_RESET -1

Adafruit_BNO08x bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;

Servo One;
float i;
float j;
float k;
float r;

int targetAngle = 50;
int errorMargin = 1;

int verticalAngle;
int currAngle;

const int PIN_VERTICAL = 5;

void setup(void) {
  One.attach(PIN_VERTICAL);

  One.write(90);
  
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("Adafruit BNO08x test!");
  if (!bno08x.begin_I2C()) {
    Serial.println("Failed to find BNO08x chip");
    while (1) { delay(10); }
  }
  Serial.println("BNO08x Found!");

  for (int n = 0; n < bno08x.prodIds.numEntries; n++) {
    Serial.print("Part ");
    Serial.print(bno08x.prodIds.entry[n].swPartNumber);
    Serial.print(": Version :");
    Serial.print(bno08x.prodIds.entry[n].swVersionMajor);
    Serial.print(".");
    Serial.print(bno08x.prodIds.entry[n].swVersionMinor);
    Serial.print(".");
    Serial.print(bno08x.prodIds.entry[n].swVersionPatch);
    Serial.print(" Build ");
    Serial.println(bno08x.prodIds.entry[n].swBuildNumber);
  }

  setReports();

  Serial.println("Reading events");
  delay(100);
}

void setReports(void) {
  Serial.println("Setting desired reports");
  if (! bno08x.enableReport(SH2_GAME_ROTATION_VECTOR)) {
    Serial.println("Could not enable game vector");
  }
}

void loop() {
  delay(50);

  if (bno08x.wasReset()) {
    Serial.print("sensor was reset ");
    setReports();
  }
  
  if (!bno08x.getSensorEvent(&sensorValue)) {
    return;
  }
  
  switch (sensorValue.sensorId)
    case SH2_GAME_ROTATION_VECTOR:
      Serial.print("Game Rotation Vector - r: ");     
      r = sensorValue.un.gameRotationVector.real;
      Serial.print(r);
      
      Serial.print(" i: ");
      i = sensorValue.un.gameRotationVector.i;
      Serial.print(i);

      verticalAngle = One.read();
      currAngle = int(i*100.00) + 90;
      if (currAngle > 180) {
        currAngle = 180;
      }
      else if (currAngle < 0) {
        currAngle = 0;
      }
      while (targetAngle != currAngle + errorMargin && targetAngle != currAngle - errorMargin) {
        if (currAngle < targetAngle) {
          verticalAngle = verticalAngle++;
        }
        else if (currAngle > targetAngle) {
          verticalAngle = verticalAngle--;
        }
        One.write(verticalAngle);
        delay(5);
        currAngle = int(i*100.00) + 90;
      }

      Serial.print(" j: ");
      j = sensorValue.un.gameRotationVector.j;
      Serial.print(j);

      Serial.print(" k: ");
      k = sensorValue.un.gameRotationVector.k;
      Serial.println(k);
}