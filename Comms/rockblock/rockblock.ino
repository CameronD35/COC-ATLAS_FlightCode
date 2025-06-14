#include <IridiumSBD.h>

#define IridiumSerial Serial1

IridiumSBD rockblock(IridiumSerial);
int signalQuality;
int err;
const char* msg = "Added signal check and loop!";

void setup() {
  Serial.begin(96000);
  IridiumSerial.begin(19200);

  Serial.println("Rockblock starting up!");

  err = rockblock.begin();

  if (err != ISBD_SUCCESS) {
    Serial.print("Rockblock failed: ");
    Serial.println(err);

    while (1);
  }

  while (signalQuality < 1) {
    err = rockblock.getSignalQuality(signalQuality);

    if (err != ISBD_SUCCESS) {
      Serial.print("Rockblock error: Couldn't report on signal quality");
      Serial.println(err);
    } else {
      Serial.print("Signal quality: ");
      Serial.println(signalQuality);
    }
  }
  Serial.println("Started up; prepping for AWS");

  err = rockblock.sendSBDText(msg);

  if (err != ISBD_SUCCESS) {
    Serial.print("Rockblock failed: "); Serial.println(err);
  } else {
    Serial.print("Text sent over Rockblock!");
    Serial.println("Check AWS!");
  }
}

void loop() {
}
