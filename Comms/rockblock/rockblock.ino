#include <IridiumSBD.h>

#define IridiumSerial Serial3

IridiumSBD rockblock(IridiumSerial);

void setup()
{
  int signalQuality = -1;
  int err;
  
  Serial.begin(96000);
  IridiumSerial.begin(19200);

  Serial.println("RockBlock Starting up...");
  err = rockblock.begin();
  if (err != ISBD_SUCCESS) {
    Serial.print("Rockblock failed:  ");
    Serial.println(err);
    if (err == ISBD_NO_MODEM_DETECTED) {
      Serial.println("Could not find RockBlock; check wiring or make sure flow control is off");
    }
    return;
  }


  // Get Firmware Version
  char version[12];
  err = rockblock.getFirmwareVersion(version, sizeof(version));
  if (err != ISBD_SUCCESS) {
     Serial.print("Couldn't get firmware version: ");
     Serial.println(err);
     return;
  }
  Serial.print("Firmware Version is ");
  Serial.print(version);

  // Get current signal quality
  // Ranges between 0-5; you want more than 1
  err = rockblock.getSignalQuality(signalQuality);
  if (err != ISBD_SUCCESS) {
    Serial.print("Failed to get signal quality: ");
    Serial.println(err);
    return;
  }

  Serial.print("Current scale is ");
  Serial.println(signalQuality);
  Serial.println("(You'll want 2 or higher).");

  // Send the message
  Serial.println("\n\nTrying to send the message. This might take a second.");
  err = modem.sendSBDText("Sending over wires--THAT I CRIMPED");
  
  if (err != ISBD_SUCCESS) {
    Serial.print("Failed to find Iridium Satellite: ");
    Serial.println(err);
    if (err == ISBD_SENDRECEIVE_TIMEOUT)
      Serial.println("Timed out--try again; make sure patch antenna is facing the sky");
  } else {
    Serial.println("Message sent!");
  }
}

void loop()
{
}
