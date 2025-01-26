// Default baud rate for RFD 900x-US
// Can and will optimize later
const long RFD_BAUD = 57600;

void setup() {
  Serial.begin(RFD_BAUD);
  Serial.println("=================================");
  Serial.println("GROUND CONTROL STARTING TO LISTEN");
  Serial.println("=================================");
}

void loop() {
  unsigned long timeSinceStart = millis();


if (Serial.available()) {
    char incomingByte = Serial.read();
    Serial.write(incomingByte); // Echo received data back
}

  if (Serial.available() > 0) {
    String received_data = Serial.readString();

    Serial.print("[Time: ");
    Serial.print(timeSinceStart);
    Serial.println(" ] Ground Control Received: ");
    Serial.println(received_data);
  }

  delay(100);
}
