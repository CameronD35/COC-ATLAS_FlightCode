#include <LiquidCrystal_I2C.h>

// This is the test set up with the LCD screen!


// Thank you for your recommendation, Owen!
#define RFSerial Serial2 

struct SensorData {
    float tempF;
    float pressure_hPa;
    float yaw;
    float pitch;
    float roll;
};

LiquidCrystal_I2C lcd(0x27,20,4);

void setup() {
  RFSerial.begin(57600);
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Snail is waking up!");

  Serial.println("Snail is waking up!");

  delay(100);
}

void loop() {
  if (RFSerial.available()) {
    SensorData receivedData;  
    RFSerial.readBytes((char*)&receivedData, sizeof(receivedData));
    lcd.clear();
    // String receivedData = RFSerial.readStringUntil('\n');
    lcd.setCursor(0,0);
    lcd.print("Temp: "); lcd.print(receivedData.tempF);

    lcd.setCursor(0,1);
    lcd.print("Pressure: "); lcd.print(receivedData.pressure_hPa);

    lcd.setCursor(0,2);
    lcd.print("Y: "); lcd.print(receivedData.yaw);
    lcd.print(", P: "); lcd.print(receivedData.pitch);
    lcd.setCursor(0,3);
    lcd.print(", R: "); lcd.print(receivedData.roll);

    Serial.print("Running");

  }
  
  delay(750);
}