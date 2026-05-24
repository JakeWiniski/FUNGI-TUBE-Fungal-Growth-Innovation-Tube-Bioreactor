/*
  FUNGI-TUBE Firmware v1.2
  Created by Jake Winiski
  Date: 24MAY2026
  Licensed under CERN-OHL-S v2.0
  See LICENSE file for details.
*/

#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include "Adafruit_MLX90614.h"
#include "Adafruit_ADS1X15.h"
#include <SensirionI2cScd4x.h>
#include "Adafruit_SGP40.h"
#include "Adafruit_SHT31.h"
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>

#define LOG_INTERVAL_MS 600000 // Logging interval in ms (recommended 10 minutes/600000 ms)
#define LED_BLINK_INTERVAL 5000 // Frequency of indicator LED blink between logging instances (recommended 5 sec/5000 ms)
#define LED_BLINK_DURATION 250 // Indicator LED on time at each blink (recommended 250 ms)
#define SCD41_WARMUP 180000 //Warmup period for SCD41 sensor (recommended 3 minutes/180000 ms)

// DS3231 RTC
RTC_DS3231 rtc;
uint32_t bootTime_epoch_s = 0;  // RTC seconds at startup

// For tracking millis() rollover
unsigned long lastMillis = 0;
uint64_t millisOffset = 0;

// SD card chip select and datafile
#define SD_CS_PIN 5
File dataFile;
bool headerChecked = false;
bool sdReady = false;

// I2C Pins
#define SDA_PIN 21
#define SCL_PIN 22

// Status LED
#define STATUS_LED_PIN 2

// I2C Multiplexer
#define PCA9548A_ADDR 0x70 
#define MUX_CH_MOISTURE 0  // Channel 0

// CO2 Sensor
#define MUX_CH_SCD41 1  // Channel 1
SensirionI2cScd4x scd41;
static int16_t error;

// RGB Sensor
#define TCS34725_INTEGRATION_TIME TCS34725_INTEGRATIONTIME_600MS
#define TCS34725_GAIN TCS34725_GAIN_16X
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATION_TIME, TCS34725_GAIN);

// IR Temp Sensor
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// ADS1115 for Capacitance Sensor
Adafruit_ADS1115 ads;
#define ADS_PGA_GAIN GAIN_TWOTHIRDS  // ±6.144V range, 1 bit ≈ 0.1875mV

// VOC and RH Sensors
#define MUX_CH_SGP40 2 // Channel 2
#define MUX_CH_SHT31 3 // Channel 3
Adafruit_SGP40 sgp;
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// Channel Selection
void selectI2CChannel(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(PCA9548A_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

// ScanI2C function for troubleshooting
void scanI2C() {
  Serial.println("I2C scan starting...");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t result = Wire.endTransmission();
    if (result == 0) {
      Serial.print("I2C device found at 0x");
      Serial.println(addr, HEX);
    }
  }
  Serial.println("Scan complete.");
}

// Data Write to SD
void logToSD(uint32_t bootTime_epoch_s, uint16_t r, uint16_t g, uint16_t b, float ambientTemp, float objectTemp, int16_t avgCapacitanceRaw,
             uint16_t co2Concentration, float co2Temp,
             uint16_t vocRaw, float vocRH, float vocTemp) {
  
  if (!sdReady) {
    Serial.println("Skipping log — SD not ready.");
    return;
  }

  unsigned long currentMillis = millis();

  // Detect millis() rollover
  if (currentMillis < lastMillis) {
    millisOffset += (uint64_t)1 << 32;  // Add 2^32 ms
  }
  lastMillis = currentMillis;

  // Compute full-time since boot
  uint64_t trueMillis = millisOffset + currentMillis;
  uint64_t currentEpochMillis = ((uint64_t)bootTime_epoch_s * 1000ULL) + trueMillis;

  // Only check for header once
  if (!headerChecked) {
    if (!SD.exists("/datalog.csv")) {
      File headerFile = SD.open("/datalog.csv", FILE_WRITE);
      if (headerFile) {
        headerFile.println("EpochMillis,R,G,B,IR_Ambient,IR_Object,CapacitanceRaw,CO2ppm,CO2Temp,VOC_Raw,VOC_RH,VOC_Temp");
        headerFile.close();
        Serial.println("Header written to SD.");
      }
    }
    headerChecked = true;
  }

  File dataFile = SD.open("/datalog.csv", FILE_APPEND);
  if (dataFile) {
    dataFile.print(currentEpochMillis); dataFile.print(",");
    dataFile.print(r); dataFile.print(",");
    dataFile.print(g); dataFile.print(",");
    dataFile.print(b); dataFile.print(",");
    dataFile.print(ambientTemp, 2); dataFile.print(",");
    dataFile.print(objectTemp, 2); dataFile.print(",");
    dataFile.print(avgCapacitanceRaw); dataFile.print(",");
    dataFile.print(co2Concentration); dataFile.print(",");
    dataFile.print(co2Temp, 2); dataFile.print(",");
    dataFile.print(vocRaw); dataFile.print(",");
    dataFile.print(vocRH, 2); dataFile.print(",");
    dataFile.println(vocTemp, 2);
    dataFile.close();
    Serial.println("Data logged to SD.");
  } else {
    Serial.println("Error opening datalog.csv");
  }
}

void setup() {
  Serial.begin(115200);
  delay(5000); // Allow ESP32 to stabilize
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(5000); // Allow I2C sensors to power up before initialization
  Serial.println("I2C bus initialized.");

  scanI2C(); // For troubleshooting

  // Initialize the RTC and Save Epoch
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting to compile time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  DateTime now = rtc.now();
  bootTime_epoch_s = now.unixtime();  // Save epoch time at boot

  uint64_t currentEpochMillis = (uint64_t)bootTime_epoch_s * 1000ULL;
  Serial.print("FUNGI-TUBE boot EpochMillis: ");
  Serial.println(currentEpochMillis);

  // Indicator LED
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  // Initialize SD card with retry logic
  Serial.print("Initializing SD card...");

  for (int i = 0; i < 3; i++) {
    if (SD.begin(SD_CS_PIN)) {
      Serial.println("SD card initialized.");
      sdReady = true;
      break;
    } else {
      Serial.print("Retry "); Serial.println(i + 1);
      delay(2000);  // Wait before retrying
    }
  }

  if (!sdReady) {
    Serial.println("SD card failed after 3 attempts.");
    Serial.println("Halting: insert card and restart device.");

    // Blink indicator LED rapidly and hold indefinitely
    pinMode(STATUS_LED_PIN, OUTPUT);
    while (true) {
      digitalWrite(STATUS_LED_PIN, HIGH);
      delay(200);  // 200 ms on
      digitalWrite(STATUS_LED_PIN, LOW);
      delay(200);  // 200 ms off
    }
  }

  // Initialize TCS34725
  Serial.println("Initializing TCS34725...");
  if (tcs.begin()) {
    Serial.println("TCS34725 initialized.");
    tcs.setInterrupt(true);
  } else {
    Serial.println("TCS34725 not found!");
  }

  // Initialize MLX90614
  Serial.println("Initializing MLX90614...");
  if (mlx.begin()) {
    Serial.println("MLX90614 initialized.");
  } else {
    Serial.println("MLX90614 not found!");
  }

  // Initialize ADS1115 via multiplexer
  Serial.print("Selecting mux channel 0 for ADS1115...");
  selectI2CChannel(MUX_CH_MOISTURE);
  delay(5);
  if (ads.begin()) {
    Serial.println("ADS1115 initialized.");
    ads.setGain(ADS_PGA_GAIN); // Set gain to ±6.144V for maximum range
  } else {
    Serial.println("ADS1115 not found!");
  }

  // Initialize SCD41 on MUX Channel 1
  Serial.print("Selecting mux channel 1 for SCD41...");
  selectI2CChannel(MUX_CH_SCD41);
  delay(30);
  scd41.begin(Wire, SCD41_I2C_ADDR_62);

  error = scd41.stopPeriodicMeasurement();  // Reset just in case
  error = scd41.wakeUp();
  delay(100);
  
  // Ensure sensor is calibrated to factory default
  uint16_t frcCorrection = 0;
  error = scd41.performForcedRecalibration(0xFFFF, frcCorrection);
  if (error) {
    Serial.print("FRC error: ");
    Serial.println(error);
  } else {
    Serial.print("SCD41: Calibration offset cleared to factory default. Correction code: ");
    Serial.println(frcCorrection);
  }

  error = scd41.setAutomaticSelfCalibrationTarget(0); // Disable ASC (for faster warmup & manual control)
  error = scd41.startPeriodicMeasurement();

  Serial.println("SCD41 warming up...");
  digitalWrite(STATUS_LED_PIN, HIGH);  // Turn on LED during warmup
  delay(SCD41_WARMUP);  // Sensor warmup, set to 3 min
  digitalWrite(STATUS_LED_PIN, LOW);  // Turn off LED
  Serial.println("SCD41 initialized.");

    // Initialize SHT31 on MUX Channel 3
  Serial.print("Selecting mux channel 3 for SHT31...");
  selectI2CChannel(MUX_CH_SHT31);
  delay(5);
  if (!sht31.begin(0x44)) {  // Use 0x45 if needed
    Serial.println("SHT31 not found!");
  } else {
    Serial.println("SHT31 initialized.");
  }

  // Initialize SGP40 on MUX Channel 2
  Serial.print("Selecting mux channel 2 for SGP40...");
  selectI2CChannel(MUX_CH_SGP40);
  delay(5);
  if (!sgp.begin()) {
    Serial.println("SGP40 not found!");
  } else {
    Serial.println("SGP40 initialized.");
  }

}

void loop() {

  digitalWrite(STATUS_LED_PIN, HIGH);  // Turn on LED
  
  // RGB Sensor Read
  tcs.enable();
  delay(500);
  tcs.setInterrupt(false);  // LED on
  delay(100);

  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);
  tcs.setInterrupt(true);   // LED off
  delay(500);

  // IR Temp Sensor Read
  float ambientTemp = mlx.readAmbientTempC();
  float objectTemp = mlx.readObjectTempC();

  // Capacitance Sensor via ADS1115
  selectI2CChannel(MUX_CH_MOISTURE);
  delay(5);
  const int numSamples = 6; // Average multiple readings to smooth sensor noise
  int32_t sumCapacitanceRaw = 0;

  for (int i = 0; i < numSamples; i++) {
    int16_t sample = ads.readADC_SingleEnded(0);  // A0 pin
    sumCapacitanceRaw += sample;
    delay(500); 
  }

  int16_t avgCapacitanceRaw = static_cast<int16_t>(sumCapacitanceRaw / numSamples);

  // NOTE:
  // The value `avgCapacitanceRaw` represents a smoothed raw capacitance signal,
  // which reflects relative dielectric changes over time (e.g., due to fungal growth).
  // While not calibrated to absolute moisture content, it is possible to do so
  // through substrate-specific calibration. This would require determining the
  // relationship between raw ADC values and gravimetric moisture content
  // across a range of known substrate moisture levels.

  // SCD41 CO₂ Sensor Read
  selectI2CChannel(MUX_CH_SCD41);
  delay(5);
  uint16_t co2Concentration = 0;
  float co2Temp = 0.0;
  float co2RH = 0.0;

  error = scd41.readMeasurement(co2Concentration, co2Temp, co2RH);

  if (error) {
  Serial.print("SCD41 read error: ");
  Serial.println(error);
  co2Concentration = 0;
  co2Temp = 0.0;
  co2RH = 0.0; // Read but unused; reserved for future use
  }

  // VOC and RH Sensor Read
  selectI2CChannel(MUX_CH_SHT31);
  delay(5);
  float vocTemp = sht31.readTemperature();
  float vocRH = sht31.readHumidity();

  selectI2CChannel(MUX_CH_SGP40);
  delay(5);
  uint16_t vocRaw = sgp.measureRaw(vocRH, vocTemp);

  // Logs all sensor readings to SD card as CSV
  // Check if header has already been written (only once per boot)
  logToSD(bootTime_epoch_s, r, g, b, ambientTemp, objectTemp, avgCapacitanceRaw,
        co2Concentration, co2Temp,
        vocRaw, vocRH, vocTemp);
  delay(500);

  // Serial Output
  Serial.print("R: "); Serial.print(r);
  Serial.print(" G: "); Serial.print(g);
  Serial.print(" B: "); Serial.print(b);
  Serial.print(" IR Ambient Temp: "); Serial.print(ambientTemp); Serial.print("°C");
  Serial.print(" IR Object Temp: "); Serial.print(objectTemp); Serial.print("°C");
  Serial.print(" Capacitance Raw: "); Serial.print(avgCapacitanceRaw);
  Serial.print(" CO2 [ppm]: "); Serial.print(co2Concentration);
  Serial.print(" CO2 Temp: "); Serial.print(co2Temp); Serial.print("°C");
  Serial.print(" VOC Raw: "); Serial.print(vocRaw);
  Serial.print(" VOC RH: "); Serial.print(vocRH);
  Serial.print(" VOC Temp: "); Serial.println(vocTemp);

  digitalWrite(STATUS_LED_PIN, HIGH);  // Turn off LED

  //Delay with LED Blinking
  //(ESP-NOW transmission could be added here)
  const unsigned long totalDelay = LOG_INTERVAL_MS;     // Delay period
  const unsigned long blinkInterval = LED_BLINK_INTERVAL;    // Blink periodicity
  const unsigned long blinkDuration = LED_BLINK_DURATION;      // LED on time

  unsigned long startTime = millis();
  while (millis() - startTime < totalDelay) {
    unsigned long elapsed = millis() - startTime;
    if ((elapsed % blinkInterval) < blinkDuration) {
      digitalWrite(STATUS_LED_PIN, HIGH);
    } else {
      digitalWrite(STATUS_LED_PIN, LOW);
    }
    delay(10); // Keeps the loop responsive
  }
  digitalWrite(STATUS_LED_PIN, LOW);  // Ensure LED is off
}
