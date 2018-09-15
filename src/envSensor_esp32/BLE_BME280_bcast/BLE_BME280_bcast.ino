/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by pcbreflux
*/

#ifdef ARDUINO_M5Stack_Core_ESP32
#include <M5Stack.h>
#endif
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "esp_sleep.h"
#include <Wire.h>
#include "bme280_i2c.h"

#define T_PERIOD     10   // Transmission perild
#define S_PERIOD     10  // Silent perild
RTC_DATA_ATTR static uint8_t seq; // remember number of boots in RTC Memory

#ifdef ARDUINO_M5Stack_Core_ESP32
#define SDA 21
#define SCL 22
#else
#define SDA 12
#define SCL 14
#endif

BME280 bme280(BME280_I2C_ADDR_PRIM);
struct bme280_data data;

void setAdvData(BLEAdvertising *pAdvertising) {
    bme280.get_sensor_data(&data);
    uint16_t temp = (uint16_t)(data.temperature * 100);
    uint16_t humid = (uint16_t)(data.humidity * 100);
    uint16_t press = (uint16_t)(data.pressure / 10);

    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();

    oAdvertisementData.setFlags(0x06); // BR_EDR_NOT_SUPPORTED | LE General Discoverable Mode

    std::string strServiceData = "";
    strServiceData += (char)0x17;   // 長さ
    strServiceData += (char)0xff;   // AD Type 0xFF: Manufacturer specific data
    strServiceData += (char)0x00;   // fake Ericsson ID low byte
    strServiceData += (char)0x00;   // fake Ericsson ID high byte
    strServiceData += (char)seq;                   // シーケンス番号をバッファーにセット
    strServiceData += (char)(temp & 0xff);         // 温度の下位バイトをセット
    strServiceData += (char)((temp >> 8) & 0xff);  // 温度の上位バイトをセット
    strServiceData += (char)(humid & 0xff);        // 湿度の下位バイトをセット
    strServiceData += (char)((humid >> 8) & 0xff); // 湿度の上位バイトをセット
    strServiceData += (char)0;
    strServiceData += (char)0;
    strServiceData += (char)0;
    strServiceData += (char)0;
    strServiceData += (char)(press & 0xff);        // 気圧の下位バイトをセット
    strServiceData += (char)((press >> 8) & 0xff); // 気圧の上位バイトをセット
    strServiceData += (char)0;
    strServiceData += (char)0;
    strServiceData += (char)0;
    strServiceData += (char)0;
    strServiceData += (char)0;
    strServiceData += (char)0;
    strServiceData += (char)0;
    strServiceData += (char)0;
    strServiceData += (char)0;

    oAdvertisementData.addData(strServiceData);
    pAdvertising->setAdvertisementData(oAdvertisementData);
}

void setup() {
#ifdef ARDUINO_M5Stack_Core_ESP32
    M5.begin();
    dacWrite(25, 0); // Speaker OFF
#endif
    pinMode(SDA, INPUT_PULLUP); // SDAピンのプルアップの指定
    pinMode(SCL, INPUT_PULLUP); // SCLピンのプルアップの指定
    Wire.begin(SDA, SCL);

    Serial.begin(115200);
    Serial.printf("start ESP32 %d\n",seq);

    bme280.begin(); // BME280の初期化

    BLEDevice::init("AmbientEnv-02");                  // デバイスを初期化
    BLEServer *pServer = BLEDevice::createServer();    // サーバーを生成

    BLEAdvertising *pAdvertising = pServer->getAdvertising(); // アドバタイズオブジェクトを取得
    setAdvData(pAdvertising);                          // アドバタイジングデーターをセット

    pAdvertising->start();                             // アドバタイズ起動
    Serial.println("Advertizing started...");
    delay(T_PERIOD * 1000);
    pAdvertising->stop();                              // アドバタイズ停止

    seq++;                                             // シーケンス番号を更新

    Serial.printf("enter deep sleep\n");
    esp_deep_sleep(1000000LL * S_PERIOD);              // Deep Sleepに移行
    Serial.printf("in deep sleep\n");
}

void loop() {
}
