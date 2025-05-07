#include <IridiumSBD.h>

// I forgot which pins this guy's hooked up to; use Serial3 for an example
#define IridiumSerial Serial3

// Can turn to true in order to view diagnostics
#define DIAGNOSTICS false

IridiumSBD modem(IridiumSerial);

void setup()
{
  int signalQuality = -1;
  int err;
  
  // Console serial
  Serial.begin(115200);
  while (!Serial);

  // RockBlock is similar to RFD; reference as "IridiumSerial"
  IridiumSerial.begin(19200);


  Serial.println("Starting modem...");
  err = modem.begin();

  if (err != ISBD_SUCCESS)
  {
    Serial.println("Failed to connect modem");
    Serial.println(err);
    
    if (err == ISBD_NO_MODEM_DETECTED)
      Serial.println("No modem detected: check wiring or change serial connection");
    return;
  }

    // Getting version just to check connection
  char version[12];
  err = modem.getFirmwareVersion(version, sizeof(version));
  if (err != ISBD_SUCCESS)
  {
     Serial.println("Failed: Unable to obtain version");
     Serial.println(err);
     return;
  }

  Serial.print("Firmware Version is ");
  Serial.print(version);
  Serial.println(".");

  // Checking connection to satellite
  // [0,5] You'll want anything more than 0

  err = modem.getSignalQuality(signalQuality);
  if (err != ISBD_SUCCESS)
  {
    Serial.println("Failed: Signal quality error");
    Serial.println(err);
    return;
  }

  Serial.print("Signal Quality [0-5]: ");
  Serial.println(signalQuality);

  // ACTUALLY SEND THE MESSAGE
  Serial.print("Sending message: Make sure I'm outside; I might take a while\r\n");
  err = modem.sendSBDText("INSERT MESSAGE HERE");
  if (err != ISBD_SUCCESS)
  {
    Serial.println("FAILED: Had trouble sending message ");
    Serial.println(err);
    if (err == ISBD_SENDRECEIVE_TIMEOUT)
      Serial.println("I timed out; please make sure my patch antenna is facing the sky and I'm outside.");
  }

  else
  {
    Serial.println("I sent it! Check your e-mail");
  }
}

void loop()
{
}

#if DIAGNOSTICS
void ISBDConsoleCallback(IridiumSBD *device, char c)
{
  Serial.write(c);
}

void ISBDDiagsCallback(IridiumSBD *device, char c)
{
  Serial.write(c);
}
#endif