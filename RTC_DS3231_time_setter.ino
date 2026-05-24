#include <Wire.h>
#include "RTClib.h"

RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin();

  Serial.println("Initializing RTC...");
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC. Check wiring.");
    while (1);
  }

  // Set RTC to the date & time this sketch was compiled
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  Serial.println("RTC time has been set to compile time.");
  delay(1000);

  // Print confirmation
  DateTime now = rtc.now();
  Serial.print("RTC is now set to: ");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.println(now.second(), DEC);
}

void loop() {
  // Do nothing
}
