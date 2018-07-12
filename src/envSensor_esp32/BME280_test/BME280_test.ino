/*
 * M5Stack/ESP32とBME280をI2C接続し、温度、湿度、気圧を測定しプリントアプトする
 */
#ifdef ARDUINO_M5Stack-Core-ESP32
#include <M5Stack.h>
#endif
#include <Wire.h>
#include "bme280_i2c.h"

#ifdef ARDUINO_M5Stack-Core-ESP32
#define SDA 21
#define SCL 22
#else
#define SDA 12
#define SCL 14
#endif

BME280 bme280(BME280_I2C_ADDR_PRIM);
struct bme280_data data;

void setup(){
#ifdef ARDUINO_M5Stack-Core-ESP32
    M5.begin();
    dacWrite(25, 0); // Speaker OFF
#endif
    pinMode(SDA, INPUT_PULLUP); // SDAピンのプルアップの指定
    pinMode(SCL, INPUT_PULLUP); // SCLピンのプルアップの指定
    Wire.begin(SDA, SCL);

    Serial.begin(115200);

    bme280.begin(); // BME280の初期化

    Serial.println("BME280 test");
#ifdef ARDUINO_M5Stack-Core-ESP32
    M5.Lcd.printf("BME280 test\r\n"); // LCD display
#endif
}

void loop() {
    bme280.get_sensor_data(&data);
    Serial.printf("temp: %.2f, humid: %.2f, press: %.1f\r\n", data.temperature, data.humidity, data.pressure / 100);

#ifdef ARDUINO_M5Stack-Core-ESP32
    M5.Lcd.printf("temp: %.2f, humid: %.2f, press: %.1f\r\n", data.temperature, data.humidity, data.pressure / 100);
#endif
    delay(5000);
}

