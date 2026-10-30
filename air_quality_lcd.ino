#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>

// Pins
#define SD_CS_PIN 5
#define MQ135_PIN 34

long prevMillis = 0;
bool displayed = false;

// Objects
RTC_DS3231 rtc;
File dataFile;

LiquidCrystal_I2C lcd(0x27, 16, 2); 

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(100); }

  Serial.println("\n--- System Init ---");

  // 1. Initialize I2C Bus (SDA = GPIO21, SCL = GPIO22)
  Wire.begin(21, 22);
  
  // Initialize LCD Screen
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Loading...      ");

  // RTC Init
  if (!rtc.begin()) {
    lcd.setCursor(0, 1);
    lcd.print("RTC Error!     ");
    while (1);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // SD Card Init
  if (!SD.begin(SD_CS_PIN)) {
    lcd.setCursor(0, 1);
    lcd.print("SD Card Error! ");
    while (1);
  }

  // Create CSV
  if (!SD.exists("/air_quality.csv")) {
    dataFile = SD.open("/air_quality.csv", FILE_WRITE);
    if (dataFile) {
      dataFile.println("Timestamp,Raw_Gas_Value");
      dataFile.close();
    }
  }

  analogReadResolution(12);                     
  analogSetPinAttenuation(MQ135_PIN, ADC_11db); 

  lcd.clear();
  Serial.println("System initialized.\n");
}

void loop() {
  int rawGasValue = analogRead(MQ135_PIN);
  DateTime now = rtc.now();

  if (now.second() == 0 && !displayed) {
    // Format Time
    char timeBuffer[20];
    sprintf(timeBuffer, "Time: %02d:%02d:%02d", now.hour(), now.minute(), now.second());
    lcd.setCursor(0, 0);
    lcd.print(timeBuffer);

    lcd.setCursor(0, 1);
    lcd.print("Writing to SD..."); 

    char timestamp[25];
    sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d", 
            now.year(), now.month(), now.day(), 
            now.hour(), now.minute(), now.second());

    dataFile = SD.open("/air_quality.csv", FILE_APPEND);
    if (dataFile) {
      dataFile.print(timestamp);
      dataFile.print(",");
      dataFile.println(rawGasValue);
      dataFile.close();

      Serial.print("Logged: ");
      Serial.print(timestamp);
      Serial.print(" | Value: ");
      Serial.println(rawGasValue);
    } else {
      Serial.println("Error writing to SD Card!");
    }

    prevMillis = millis();
    displayed = true;
  } else {
    if (now.second() == 1) {
      displayed = false;
    }
    if (millis() >= prevMillis + 1000) {
      prevMillis = millis();
      // Format Time
      char timeBuffer[20];
      sprintf(timeBuffer, "Time: %02d:%02d:%02d", now.hour(), now.minute(), now.second());
      lcd.setCursor(0, 0);
      lcd.print(timeBuffer);

      // Format Gas Value
      lcd.setCursor(0, 1);
      lcd.print("Air Qual:       "); 
      lcd.setCursor(10, 1);
      lcd.print(rawGasValue);

      delay(10);
    }
  }
}
