/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini

    オムロン2JCIE_BL01のGATTに合わせて温度、湿度、気圧データーを送る
    readがあったらBME280を読み、温度、湿度、気圧データーをsetValue()するCallbackを設定する
*/
#ifdef ARDUINO_M5Stack_Core_ESP32
#include <M5Stack.h>
#endif
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <esp_wifi.h>
#include <esp_gatts_api.h>
#include <Wire.h>
#include "bme280_i2c.h"

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SENSOR_UUID     "b0c8be70-6d46-11e8-adc0-fa7ae01bbebc"
#define LATESTDATA_UUID "b0c8c0fa-6d46-11e8-adc0-fa7ae01bbebc"

#ifdef ARDUINO_M5Stack_Core_ESP32
#define SDA 21
#define SCL 22
#else
#define SDA 12
#define SCL 14
#endif

BME280 bme280(BME280_I2C_ADDR_PRIM);
struct bme280_data data;

void bdaDump(esp_bd_addr_t bd) {
    for (int i = 0; i < ESP_BD_ADDR_LEN; i++) {
        Serial.printf("%02x", bd[i]);
        if (i < ESP_BD_ADDR_LEN - 1) {
            Serial.print(":");
        }
    }
};

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t* param) {
        Serial.print("connected from: ");
        bdaDump(param->connect.remote_bda);
        Serial.println("");

        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        conn_params.latency = 0;
        conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
        //start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);
    };

    void onDisconnect(BLEServer* pServer) {
        Serial.println("disconnected");
    }
};

uint8_t seq = 0;

class dataCb: public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic *pChar) {
        uint8_t buf[7];

        bme280.get_sensor_data(&data);　　　　　　　// センサーを読む
        Serial.printf("temp: %f, humid: %f, press: %f\r\n", data.temperature, data.humidity, data.pressure / 100);
        uint16_t temp = (uint16_t)(data.temperature * 100);
        uint16_t humid = (uint16_t)(data.humidity * 100);
        uint16_t press = (uint16_t)(data.pressure / 10);
        memset(buf, 0, sizeof buf);               // バッファーを0クリア
        buf[0] = seq++;                           // シーケンス番号をバッファーにセット
        buf[1] = (uint8_t)(temp & 0xff);          // 温度の下位バイトをセット
        buf[2] = (uint8_t)((temp >> 8) & 0xff);   // 温度の上位バイトをセット
        buf[3] = (uint8_t)(humid & 0xff);         // 湿度の下位バイトをセット
        buf[4] = (uint8_t)((humid >> 8) & 0xff);  // 湿度の上位バイトをセット
        buf[5] = (uint8_t)(press & 0xff);         // 気圧の下位バイトをセット
        buf[6] = (uint8_t)((press >> 8) & 0xff); // 気圧の上位バイトをセット
        pChar->setValue(buf, sizeof buf);         // データーを書き込み
    }
};

void setup() {
#ifdef ARDUINO_M5Stack_Core_ESP32
    M5.begin();
    dacWrite(25, 0); // Speaker OFF
#endif
    pinMode(SDA, INPUT_PULLUP); // SDAピンのプルアップの指定
    pinMode(SCL, INPUT_PULLUP); // SCLピンのプルアップの指定
    Wire.begin(SDA, SCL);

    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    bme280.begin(); // BME280の初期化

    BLEDevice::init("AmbientEnv-01");                  // デバイスを初期化
    BLEServer *pServer = BLEDevice::createServer();    // サーバーを生成
    pServer->setCallbacks(new MyServerCallbacks());    // コールバック関数を設定

    BLEService *pService = pServer->createService(SENSOR_UUID);  // サービスを生成
                                                                 // キャラクタリスティクスを生成
    pService->createCharacteristic(LATESTDATA_UUID, BLECharacteristic::PROPERTY_READ)
        ->setCallbacks(new dataCb());                  // コールバック関数を設定

    pService->start();                                 // サービスを起動
    pServer->getAdvertising()->start();                // アドバタイズを起動
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
}
