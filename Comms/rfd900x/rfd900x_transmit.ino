#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <Wire.h> // Include wire library

// Create sensor objects
Adafruit_BME280 bme;  // Use default I2C bus
Adafruit_BMP280 bmp(&Wire1); // Use second I2C bus

void setup() {
  Serial.begin(115200);
  Serial1.begin(57600);

  while (!Serial); // Wait for serial monitor
  Serial.println("Initializing sensors...");


  // Initialize BMP280 on second I2C bus (Pins 17 and 16)
  if (!bmp.begin(0x76)) { // Use appropriate I2C address
    Serial.println("Could not find a valid BMP280 sensor!");
    while (1);
  }

  Serial.println("BMP, sure. That makes sense.");

  // Initialize BME280 on default I2C bus (Pins 18 and 19)
  if (!bme.begin(0x77)) { // Use appropriate I2C address
    Serial.println("Could not find a valid BME280 sensor!");
    while (1);
  }



  Serial.println("Sensors initialized!");
}

void loop() {
  // Read data from BME280
  float bmeTemp = bme.readTemperature();
  float bmeHum = bme.readHumidity();
  float bmePres = bme.readPressure() / 100.0F; // Convert to hPa

  // Read data from BMP280
  float bmpTemp = bmp.readTemperature();
  float bmpPres = bmp.readPressure() / 100.0F; // Convert to hPa

  // Format data into a string
  String dataPacket = String("{'bme_temp_c': ") + bmeTemp + ", 'bme_hum_%': " + bmeHum + ", 'bme_pres_hPa': " + bmePres + ", " +
                      "'bmp_temp_c': " + bmpTemp + ", 'bmp_pres_hPa': " + bmpPres + "}";

  // Send data over RFD900x
  Serial1.println(dataPacket);

  // Debugging
  Serial.println(dataPacket);

  delay(1000); // Send data every second
}