// Default baud rate for RFD900x-US
// Can and will optimize at a later date
const long RFD_BAUD = 57600;

void setup() {
    Serial.begin(RFD_BAUD);
    Serial.println("===================================");
    Serial.println("MAJOR TOM STARTING TO SING");
    Serial.println("===================================");
}

void loop(){
    unsigned long timeSinceStart = millis();
    Serial.print("[Time: ");
    Serial.print(timeSinceStart);
    Serial.println(" ] This is Major Tom to Ground Control");
    delay(1000);
}