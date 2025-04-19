// time between each rotation
// According to the RSX user guide an ejection/extensions speed less
// than 1 in/sec. All threads are 10 thread/in and therefore only
// 10 rotations can happen a second (1 / 10 = 0.1)
// #define maxTimeBtwnRotation_ms 100

const short leadscrewStepperPin = 24;
const short pharosRotationStepperPin = 14;
const short pharosServoPin = 23; // Lebron James
const int stepsPerRotation = 200;

const int TE1PIN = 20;
const int TE3PIN = 40;

File dataFile;

const int radioSerialPin = 27;
//HardwareSerial radioSerial = Serial1;

int powerUpCompleted = 0;
int ejectionBegun = 0;


// https://github.com/janelia-arduino/TMC2209
#include <TMC2209.h>
TMC2209 stepper_driver;
HardwareSerial & serial_stream = Serial1;
const long SERIAL_BAUD_RATE = 112500;

#include <stdbool.h>
#include <string.h>
#include <SD.h>

// servo library
#include <Servo.h>
Servo pharosServo;

// Rotates stepper
// driver (TMC2209) - reference to TMC2209 driver
// pin (short) - pin number for the stepper driver
// rotations (int) - number of desired rotations
// time_sec (float) - amount of time to conduct 'rotations' in
int rotateStepper(TMC2209 driver, short pin, int rotations, float time_sec, bool direction) {

    // rotations per second -> delay between steps (microseconds)
    float rotationsPerSec = rotations / time_sec;

    float timeBtwnSteps_usec = 1000000 / (rotationsPerSec * stepsPerRotation);
    
    // round 
    timeBtwnSteps_usec = round(timeBtwnSteps_usec);
    
   
    if (timeBetweenRotations < maxTimeBtwnRotation_ms) {
        return -1;
    }

    if (direction) {
        // code for rotating counterclockwise
    } else {
        // code for rotating clockwise
    }
    
    // number of steps completed
    int steps = 0;

    
    for (int i = 0; i < rotations*stepsPerRotation; i++) {

      digitalWrite(pin, HIGH);
      delayMicroseconds(timeBtwnSteps_usec/2);

      digitalWrite(pin, LOW);
      delayMicroseconds(timeBtwnSteps_usec/2);

      steps++;
    }

    return 0;

}

int rotateServo(int servo, int rotationDegree, float time_sec, bool direction) {
  int currentAngle = 0;

  // calculate delay time based on time_sec

  for (int i = 0; i < rotationDegree; i++) {

    servo.write(i);
    delay(10);

  }

}

String gatherRadioData(HardwareSerial radioSerial) {
  
  // WORK
  Serial.println("Snail is listening\n");
  
  // Check if data is available from RFD900x
  int dataInBuffer = radioSerial.available();
  String recievedData;
  String timeStamp;
  String fullData;


  if (dataInBuffer > 0) {

    receivedData = radioSerial.readStringUntil('\n'); // Read incoming data

    // PLEASE WORK
    Serial.println("Snail received message!\n");

    // Time of data recieved
    timeStamp = String(hour()) + ":" + String(minute()) + ":" + String(second()) + " " +
                       String(day()) + "/" + String(month()) + "/" + String(year());

    // IT WORKED
    Serial.println("Received: " + receivedData);

    // Data string and timestamp together (in JSON format)
    fullData = "{'timestamp': " + timeStamp + ", 'data': " + receivedData + " }";

  } else {

    Serial.println("No data :(\n");

  }

  // do anything with the data? WRITE STUFF

  writeToFile(dataFile, fullData);
  

  return fullData;

}

int writeToFile(File file, String data) {

  // Open file
  // File dataFile = SD.open(file, FILE_WRITE);

  // Check to see if the file opened
  if (dataFile) {

    // write to the file
    dataFile.println(data);

    // flush all the data in memory
    dataFile.flush();

    // Print the saved data to the Serial Monitor
    Serial.println("Data received and saved: " + data);

  } else {
    // :(((((
    Serial.println("Error opening file!");
    
  }

}

void setup() {

  // setup TE-1 pin to read for event signal
  pinMode(TE1PIN, OUTPUT);

  // setup TE-3 pin to read for event signal
  pinMode(TE3PIN, OUTPUT);

  // Open file
  dataFile = SD.open(filename, FILE_WRITE);

  // attach servo to pin
  pharosServo.attach(pharosServoPin);

  // setup driver
  stepper_driver.setup(Serial1, SERIAL_BAUD_RATE);

  // standard Serial
  Serial.begin(115200);

  // radio Serial
  Serial2.begin(57600);

}

void loop() {
  // put your main code here, to run repeatedly:

  int TE1Value = digitalRead(TE1PIN);
  int TE3Value = digitalRead(TE3PIN);

  if (TE3Value == HIGH && !powerUpCompleted) {

    powerUpCompleted = 1;
    // Capsule Powerup
    
    Serial.println("TE-3 IS A GO.");

  }

  if (TE1Value == HIGH && !ejectionBegun) {

    Serial.println("TE-1 IS A GO.");

    ejectionBegun = 1;


    // error handling
    if (servoRotate == -1) {

      Serial.println("Error while rotating servo.");

    }

    int stepperRotate = rotateStepper(stepper_driver, leadscrewStepperPin, 65, 6.5, 1);

    // error handling
    if (stepperRotate == -1) {

      Serial.println("Error while rotating stepper.");

    }
    //int unStepperRotate = rotateStepper(stepper_driver, leadscrewStepperPin, 65, 6.5, -1);
    int currentAngle = pharosServo.read();

    // This is an arbitary number for now
    if (currentAngle <= 20) {

      int servoRotate = rotateServo(pharosServo, 90, 1, 1);

    }



  }
  
  // Collect data from radio
  String data = gatherRadioData(Serial2);

  delay(50);

}