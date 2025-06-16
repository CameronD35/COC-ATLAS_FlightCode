/*
  PHAROS Capsule Code
  --------------------------------
  - Gathers data from sensors (temperature, pressure, IMU)
  - Sends data via RFD900x (to RFD900x on payload) and RockBlock 9603 (to Iridium Satellite Network)
  - Uses Message Pack to compress messages down to binary
  - Runs data collection, servo / gyro, and data transmission "simultaneously" using TeensyThreads

  Threads:
    1. Sensor and RFD Thread: Collects sensor data, sends via RFD 900x
    2. RockBlock Thread: Sends latest data via Iridium Satellite (uses data gathered by Sensor/RFD thread)
    3. Servo Thread: Placeholder for actual servo gyro code
*/

#include <Wire.h>
// Sensors
#include <Adafruit_MCP9808.h>
#include <Adafruit_BNO08x.h>
#include "Adafruit_MPRLS.h"
// Data Transmission
#include <ArduinoJson.h>
#include <MsgPack.h>
#include <IridiumSBD.h>
// Servo
#include <Servo.h>
// Threads
#include <TeensyThreads.h>

// Hardware/Pin definitions
#define DATA_BUFFER_SIZE 256
#define SERVO_PIN 22
#define RFSerial Serial1
#define IridiumSerial Serial2

// Global Flags
bool PRINT_DATA = true;
bool USE_RFD = true;
bool USE_ROCKBLOCK = true;
bool USE_SERVO = true;

// Sensors and actuators
Servo pharosServo;
Adafruit_MCP9808 tempSensor = Adafruit_MCP9808();
Adafruit_MPRLS pressureSensor = Adafruit_MPRLS();
Adafruit_BNO08x imuSensor;
IridiumSBD rockblock(IridiumSerial);

// IMU related variables
sh2_SensorValue_t sensorValue;
sh2_SensorId_t reportType = SH2_ARVR_STABILIZED_RV;
long reportIntervalUs = 5000;
int signalQuality = -1;
int err; 

// Thread related variables
// Sensor/RFD and RockBlock are able to share variables
// since they're using volatiles and mutex
volatile uint8_t sharedData[DATA_BUFFER_SIZE];
volatile size_t sharedDataLen = 0;
Threads::Mutex dataMutex;
volatile bool newDataAvailable = false; // Flag to let the RockBlock Thread know there's data available

// setReports() : Configures IMU to report sepecific events at a specific interval
void setReports(sh2_SensorId_t reportType, long reportInterval) {
  if (!imuSensor.enableReport(reportType, reportInterval)) {
    Serial.println("IMU ERROR - Set reports failed");
  }
}

// threadSensorAndRFD() : The first thread; runs twice per second
//    1. Reads all sensors (temperature, pressure, IMU)
//    2. Serializes data using MsgPack
//    3. Sends the data via RFD900x radio link
//    4. Updates the shared buffer for RockBlock thread.
void threadSensorAndRFD() {
  while (true) {
    // Only runs when the IMU has data since it's the slowest
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
      
      // Serializes data to MessagePack binary buffer
      MsgPack::Packer packer;
      packer.serialize(sensorData);
      const uint8_t* msgpackBuffer = packer.data();
      size_t len = packer.size();

      if (USE_RFD) {
        // Ensures binary delimited by length
        // Done to prevent partial data from being received and corrupting things
        uint16_t protocol_len = packer.size();
        RFSerial.write((uint8_t*)&len, 2);
        // Actually sends the Message Pack buffer containing data over
        RFSerial.write(msgpackBuffer, len);
      }

      // Shares the updated buffer with RockBlock thread
      dataMutex.lock();
      memcpy((void*)sharedData, msgpackBuffer, len);
      sharedDataLen = len;
      newDataAvailable = true;
      dataMutex.unlock();

      // Just prints data for debugging--feel free to erase!
      if (PRINT_DATA) {
        Serial.print("MsgPack:");

        for (size_t i = 0; i < len; i++) {
          Serial.print(msgpackBuffer[i], HEX); Serial.print(" ");
        }

        Serial.println();
      }// end PRINT_DATA
    } // end IMU sensor check
    
    // Forces wait before collecting next sample
    // Allows other threads to do their thing in the meantime
    threads.delay(500);
  }
}

// threadRockblock() : Sends any buffered data to the Iridium Satellite Network
//    1. Waits to see if there's new data
//    2. Checks Iridium Signal Quality (otherwise the sendBinary takes FOREVER)
//    3. If signal is good, sends MessagePack data over to Iridium Satellite Network
//    4. Runs every 500 ms
void threadRockblock() {
  while (true) {
    bool sendNow = false;
    size_t len;
    uint8_t buffer[DATA_BUFFER_SIZE];

    // Locks and retrieves the latest data from the shared data buffer
    dataMutex.lock();

    if (newDataAvailable) {
      len = sharedDataLen;
      memcpy(buffer, (const void*)sharedData, len);
      newDataAvailable = false;
      sendNow = true;
    }

    dataMutex.unlock();

    // If there is data, we check the signal quality to see if we should send
    if (sendNow) {
      signalQuality = -1;
      err = rockblock.getSignalQuality(signalQuality);

      // Rockblock check signal quality - only send when 2 or higher
      // SignalQuality ranges from 0 to 5; 5 is optimal; 1 is OKAY, but the documentation recommended 2
      if (signalQuality > 1 && err == ISBD_SUCCESS) {
        // Added for debugging reasons; sometimes SendSBDBinary takes FOREVER so I wanted to see when the send iniates to try and optimise
        Serial.println("RockBlock signal greater than 1; trying to send; might take a sec.");
        // This is what actually sends the data over, by the way!
        err = rockblock.sendSBDBinary((const uint8_t*)buffer, len);

        if (err == ISBD_SUCCESS) {
          if (PRINT_DATA) Serial.println("Message sent! Check AWS!");
        } else {
          if (PRINT_DATA) Serial.print("Unable to send message: "); Serial.println(err);
        } 

      } else {
        if (PRINT_DATA) Serial.println("RockBlock: No signal; Point patch antenna toward sky for better reception");
      } // End signal quality check
    } // End check for whether the signal should be sent
    
    // Checks for new data twice per second
    // In practise, it typically sends once per 40 ms when the signal is good
    // This could probably be sped up by decreasing the signalQuality required to send to 1+ instead of 2+
    threads.delay(500);
  }
}

// TODO: REPLACE THE CODE IN MOVECAPSULE WITH THE CAPSULE GYRO CODE
// Currently, moveCapsule() just contains code that rotates the servo back and forth continually
// This isn't the final capsule code--we need to replace it for whatever was used for the capsule test
// Place the code inside this function; do not delete
// It's called by the Servo Thread.
void moveCapsule(){
  // Once again, these loops are dummy code; please replace with actual PHAROS Capsule gyro code
  // I just used these loops to make sure the servo's function wasn't being interrupted by my code
  for (int i = 0; i <= 180; i++) {
    pharosServo.write(i);
    delay(15);
  }

  for (int i = 180; i >= 0; i--) {
    pharosServo.write(i);
    delay(15);
  }
}

// threadMoveCapsule() : The Thread that manages servo movement.
//    1. Moves the capsule
//    2. Runs independently of sensor and rockblock threads
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

      // Currently turns RockBlock off if it can't connect
      USE_ROCKBLOCK = false;
    }
  }

  if (USE_SERVO) {
    pharosServo.attach(SERVO_PIN);
  }
  
  if (!tempSensor.begin(0x19) && PRINT_DATA) Serial.println("Failed to find Temperature Sensor");

  if (!pressureSensor.begin() && PRINT_DATA) Serial.println("Failed to find Pressure Sensor");
  
  if (!imuSensor.begin_I2C() && PRINT_DATA) Serial.println("Failed to find IMU");

  setReports(reportType, reportIntervalUs);

  // Starts all threads after everything initialized
  threads.addThread(threadMoveCapsule);
  threads.addThread(threadSensorAndRFD);
  threads.addThread(threadRockblock);
}

void loop() {
  // Since we're using threads, there's not need for a main loop
}
