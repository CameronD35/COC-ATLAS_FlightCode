// time between each rotation
// According to the RSX user guide an ejection/extensions speed less
// than 1 in/sec. All threads are 10 thread/in and therefore only
// 10 rotations can happen a second (1 / 10 = 0.1)
#include <stdbool.h>
#include <string.h>
#include <SD.h>
#include <TimeLib.h>

// servo library
#include <Servo.h>

#define PH_DIR_PIN 25
#define PH_STEP_PIN 24 // Kobe Bryant
#define PH_EN_PIN 23 // Lebron James
#define PH_SERV_PIN 27 // I cannot think of a basketball player for this one

#define AT_DIR_PIN 40
#define AT_STEP_PIN 39
#define AT_EN_PIN 38

//#define RFD_SER_PIN 27

#define TE1PIN 20 // NOT FINAL
#define TE3PIN 40 // NOT FINAL

#define STEPS_PER_ROTATION 200

#define SERIAL_BAUD_RATE 112500

//File dataFile;

int powerUpCompleted = 0;
int ejectionBegun = 0;

const int pharosServoStartAngle = 90;
const int pharosServoEndAngle = 0;

Servo pharosServo;

int setupStepper(int STEP_PIN, int DIR_PIN, int ENABLE_PIN) {

  // Let the stepper pin output voltage
  pinMode(STEP_PIN, OUTPUT);

  // Let the direction pin output voltage
  pinMode(DIR_PIN, OUTPUT);

  // Let the enable pin output voltage
  pinMode(ENABLE_PIN, OUTPUT);

  // To prevent the stepper motor from overheating, we have to set the enable pin to high (disables stepping)
  digitalWrite(ENABLE_PIN, HIGH);

  return 0;

}

// Rotates stepper
// pin (short) - pin number for the stepper driver
// rotations (int) - number of desired rotations
// time_sec (float) - amount of time to conduct 'rotations' in
int rotateStepper(int STEP_PIN, int DIR_PIN, int EN_PIN, int rotations, float time_sec, bool direction) {

  Serial.println(STEP_PIN);
  Serial.println(DIR_PIN);
  Serial.println(EN_PIN);


  digitalWrite(EN_PIN, LOW);

    // Calculates the steps per second
    // ex: if we wish to have 10 rotations a second, and each full rotation requires 200 steps,
    // we would need 2000 steps per second
    int totalSteps = rotations * STEPS_PER_ROTATION;

    int stepsPerSec = totalSteps / time_sec;

    // 1 million microseconds per second
    // The delay between each step must be split between the delay from low -> high and high -> low
    // Thus, we can achieve this by saying we have two times the steps per second
    int timeBtwnPwrSwitch = 1000000 / (stepsPerSec * 2);


    if (direction) {

        Serial.println("ccw");

        digitalWrite(DIR_PIN, HIGH);

    } else {

        Serial.println("cw");
        
        digitalWrite(DIR_PIN, LOW);
        
    }
    

    // number of steps completed
    int steps = 0;

    
    for (int i = 0; i < totalSteps; i++) {

      //Serial.println("step");

      digitalWrite(STEP_PIN, HIGH);
      delayMicroseconds(timeBtwnPwrSwitch);

      digitalWrite(STEP_PIN, LOW);
      delayMicroseconds(timeBtwnPwrSwitch);

      steps++;

    }

    digitalWrite(EN_PIN, HIGH);

    Serial.println(steps);

    return 0;

}

// Comment from https://arduino.stackexchange.com/questions/46211/how-to-pass-serial-as-an-input-argument-in-arduino-function
// Print -> Stream -> HardwareSerial => [Serial]
// We use a reference to a Stream object here instead of the object itself
String gatherRadioData(Stream &radioSerial, File dataFile) {
  
  // WORK
  Serial.println("Snail is listening\n");

  String recievedData;
  String timestamp;
  String fullData;

  // Check if data is available from RFD900x
  int dataInBuffer = radioSerial.available();

  if (dataInBuffer > 0) {

    recievedData = radioSerial.readStringUntil('\n'); // Read incoming data

    // PLEASE WORK
    Serial.println("Snail received message!\n");

    // Time of data recieved
    timestamp = String(hour()) + ":" + String(minute()) + ":" + String(second()) + " " +
                       String(day()) + "/" + String(month()) + "/" + String(year());

    // IT WORKED
    Serial.println("Received: " + recievedData);

    // Data string and timestamp together (in JSON format)
    fullData = "{'timestamp': " + timestamp + ", 'data': " + recievedData + " }";

  } else {

    Serial.println("No data :(\n");

  }

  // do anything with the data? WRITE STUFF

  // writeToFile(dataFile, fullData);
  

  return fullData;

}

int writeToFile(char* file, String data) {

  // Open file
  File dataFile = SD.open(file, FILE_WRITE);

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
  
  return 0;

}

void setup() {

  // setup TE-1 pin to read for event signal
  pinMode(TE1PIN, OUTPUT);

  // setup TE-3 pin to read for event signal
  pinMode(TE3PIN, OUTPUT);

  // Open file
  File dataFile = SD.open("coolfilename.txt", FILE_WRITE);

  // setup pharos rotation stepper
  setupStepper(PH_STEP_PIN, PH_DIR_PIN, PH_EN_PIN);

  // attach servo to pin
  pharosServo.attach(PH_SERV_PIN);

  // standard Serial
  Serial.begin(115200);

  // radio Serial
  Serial2.begin(57600);

}

void loop() {
  // put your main code here, to run repeatedly:

  //Serial.println("Checking events.");

  // grab state of the TE1 and TE3 pins
  int TE1Value = digitalRead(TE1PIN);
  int TE3Value = digitalRead(TE3PIN);

  // USED FOR TESTING WITHOUT TE PINS CONNECTED
  // TE1Value = HIGH;
  // TE3Value = HIGH;

  // Ensures that the TE3 sequence will only run once
  if (TE3Value == HIGH && !powerUpCompleted) {

    powerUpCompleted = 1;
    
    // Capsule Powerup
  
    Serial.println("TE-3 IS A GO.");

  }

  
  // Ensures that the TE1 sequence will only run once
  if (TE1Value == HIGH && !ejectionBegun) {

    Serial.println("TE-1 IS A GO.");

    ejectionBegun = 1;

    Serial.print("begin angle: ");
    Serial.println(pharosServo.read());

    // Tells the servo connected to the mechanical inhibit to move to 0 degrees (incase it has moved due to vibrations)
    pharosServo.write(pharosServoStartAngle);

    delay(500);

    Serial.print("end angle: ");
    Serial.println(pharosServo.read());

    // Tells the servo connected to the mechanical inhibit to move to 90 degrees
    // Once done, the capsule is no longer mechanically inhibited
    pharosServo.write(pharosServoEndAngle);

    // PHAROS

    // Rotate ejection stepper code
    // forwards
    // 250 rot = 5.1 cm
    // 610 with capsule
    int ejectCapsule = rotateStepper(PH_STEP_PIN, PH_DIR_PIN, PH_EN_PIN, 610, 13, 1);

    // // error handling
    if (ejectCapsule == -1) {

      Serial.println("Error while ejecting capsule.");

    }

    //Retract ejection stepper code
    // backwards
    int retractEjectionSys = rotateStepper(PH_STEP_PIN, PH_DIR_PIN, PH_EN_PIN, 610, 13, 0);

    if (retractEjectionSys == -1) {

      Serial.println("Error while retracting ejection system.");

    }
    
    // ATLAS
  
    int altasStepperExtend = rotateStepper(AT_STEP_PIN, AT_DIR_PIN, AT_EN_PIN, 100, 13, 1);

    // error handling
    if (altasStepperExtend == -1) {

      Serial.println("Error while extending ATLAS housing.");

    }

    // Retract ejection stepper code
    // backwards
    int altasStepperRetract = rotateStepper(AT_STEP_PIN, AT_DIR_PIN, AT_EN_PIN, 100, 13, 0);

    if (altasStepperRetract == -1) {

      Serial.println("Error while retracting ATLAS housing.");

    }


  }
  
  // Collect data from radio
  //String data = gatherRadioData(*Serial2);

  delay(500);

}