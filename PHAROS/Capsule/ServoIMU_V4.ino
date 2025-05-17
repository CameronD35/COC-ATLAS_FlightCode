#include <Adafruit_BNO08x.h>
#include <sh2.h>
#include <sh2_SensorValue.h>
#include <sh2_err.h>
#include <sh2_hal.h>
#include <sh2_util.h>
#include <shtp.h>
#include <Servo.h>
#include <math.h>

// Don't change these
#define BNO08X_CS 10
#define BNO08X_INT 9
#define BNO08X_RESET -1

// Don't change these
Adafruit_BNO08x bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;

// Initializes Servo and IMU variables
Servo One;
float i;
float j;
float k;
float r;

// Declares time (in ms) between checking position
int checkPositionDelay = 50;

// Declares time (in ms) between each degree of movement (Speed of movement)
int movementDelay = 5;

// Determines whether IMU data should be printed
bool printData = true;

// Determines whether movement is enabled, mostly intended for testing
bool enableMovement = true;

// Declares target angle and error margin
int targetAngle = 50;
int errorMargin = 1;

// Track servo and capsule angles
int servoAngle;
int currAngle;

// Servo pin
const int SERVO_PIN = 5;

// Stores user input during execution
String input;
int temp;

void setup(void) {
  // Attaches Servo to pin
  One.attach(SERVO_PIN);
  
  // Sets Servo to 90 degrees, can be removed
  One.write(90);
  
  // Initializes IMU serial communication
  Serial1.begin(115200);
  while (!Serial1) delay(10);
  Serial.println("Adafruit BNO08x test!");
  if (!bno08x.begin_I2C()) {
    Serial.println("Failed to find BNO08x chip");
    while (1) { delay(10); }
  }
  Serial.println("BNO08x Found!");

  // Prints IMU specs, can be removed
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
  // Tells IMU to start reading values
  setReports();

  // Can be removed
  Serial.println("Reading events");
  delay(100);

  // Reads user data
  Serial2.begin(9600);
}

void setReports(void) {
  // Tells IMU to start reading values
  Serial.println("Setting desired reports");
  if (! bno08x.enableReport(SH2_GAME_ROTATION_VECTOR)) {
    Serial.println("Could not enable game vector");
  }
}

void loop() {
  // Delay between loops
  delay(checkPositionDelay);
  
  // Resets postion when sensor gets reset
  if (bno08x.wasReset()) {
    Serial.print("sensor was reset ");
    setReports();
  }
  
  // Exits loop if IMU stops reading data
  if (!bno08x.getSensorEvent(&sensorValue)) {
    return;
  }

  // Checks if data should be printed
  if (printData) {
    printIMUData();
  }

  // Checks if servo should move capsule
  if (enableMovement) {
    moveCapsule();
  }

  // Reads user input to access functions during execution
  if (Serial2.available()) {
    input = String(Serial2.peek());
    // Toggles data printing to console
    if (input.equals("D") || input.equals("d")) {
      togglePrint();
      Serial2.read();
    }
    // Toggles movement of capsule
    else if (input.equals("C") || input.equals("c")) { 
      toggleMovement();
      Serial2.read();
    }
    // Sets new target angle
    else if (input.equals("T") || input.equals("t")) {
      input = Serial2.readString();
      input = input.substring(2);
      temp = input.toInt();
      updateTargetAngle(temp);
    }
    // Sets new error margin
    else if (input.equals("E") || input.equals("e")) {
      input = Serial2.readString();
      input = input.substring(2);
      temp = input.toInt();
      updateErrorMargin(temp);
    }
    // Sets new movement delay
    else if (input.equals("M") || input.equals("m")) {
      input = Serial2.readString();
      input = input.substring(2);
      temp = input.toInt();
      updateMovementDelay(temp);
    }
    // Sets new position update delay
    else if (input.equals("P") || input.equals("p")) {
      input = Serial2.readString();
      input = input.substring(2);
      temp = input.toInt();
      updatePositionDelay(temp);
    }
  }
}

// Determines whether movement is enabled
void toggleMovement() {
  if (enableMovement) {
    enableMovement = false;
  }
  else {
    enableMovement = true;
  }
}

// Toggles whether data is printed
void togglePrint() {
  if (printData) {
    printData = false;
  }
  else {
    printData = true;
  }
}

// Prints positional data from IMU
void printIMUData(void) {
  // Reads sensor data
  switch (sensorValue.sensorId) {
    case SH2_GAME_ROTATION_VECTOR:
    
    // Prints r (Not entirely sure what r represents tbh, but not necessary for gyration)
    Serial.print("Game Rotation Vector - r: ");     
    r = sensorValue.un.gameRotationVector.real;
    Serial.print(r);
    
    // Prints i-axis position  
    Serial.print(" i: ");
    i = sensorValue.un.gameRotationVector.i;
    Serial.print(i);

    // Prints j-axis position
    Serial.print(" j: ");
    j = sensorValue.un.gameRotationVector.j;
    Serial.print(j);

    // Prints k-axis position
    Serial.print(" k: ");
    k = sensorValue.un.gameRotationVector.k;
    Serial.println(k);
    break;
  }
}

// In milliseconds
void updatePositionDelay(int n) {
  checkPositionDelay = n;
  Serial.println("Positional update delay now " + String(n) + " milliseconds.");
}

// In milliseconds
void updateMovementDelay(int n) {
  movementDelay = n;
  Serial.println("Angular movement delay now " + String(n) + " milliseconds.");
}

// In degrees
void updateTargetAngle(int n) {
  targetAngle = n;
  Serial.println("Target angle now " + String(n) + " degrees.");
}

// In degrees
void updateErrorMargin(int n) {
  errorMargin = n;
  Serial.println("Error margin now " + String(n) + " degrees.");
}

// Controls servo/capsule movement
void moveCapsule() {
  // Checks IMU data
  switch (sensorValue.sensorId) {
    case SH2_GAME_ROTATION_VECTOR:
    
    // Assigns i position to variable
    i = sensorValue.un.gameRotationVector.i;
    
    // Reads initial Servo angle
    servoAngle = One.read();

    // Reads current capsule angle
    currAngle = int(i*100.00) + 90;

    // Accounts for inaccuracies near edges
    if (currAngle > 180) {
      currAngle = 180;
    }
    else if (currAngle < 0) {
      currAngle = 0;
    }

    // Checks capsule position in relation to target angle
    while (targetAngle > currAngle + errorMargin || targetAngle < currAngle - errorMargin) {
      // Increases angle
      if (currAngle < targetAngle) {
        servoAngle++;
      }
      // Decreases angle
      else if (currAngle > targetAngle) {
        servoAngle--;
      }

      // Moves servo
      One.write(servoAngle);
      // Delay before next movement
      delay(movementDelay);
      // Updates capsule angle for accuracy
      i = sensorValue.un.gameRotationVector.i;
      currAngle = int(i*100.00) + 90;
    }
  }
}