/**
 * setup()でBLEとWiFiを初期化。
 * loop()でscan()して、デバイスを見つけたら、センサーデーターを取り出し、
 * Ambientに送信
 */
#ifdef ARDUINO_M5Stack_Core_ESP32
#include <M5Stack.h>
#endif
#include "BLEDevice.h"
#include "Ambient.h"

uint8_t seq; // remember number of boots in RTC Memory
#define MyManufacturerId 0xffff  // test manufacturer ID

WiFiClient client;
const char* ssid = "ssid";
const char* password = "password";

Ambient ambient;
unsigned int channelId = 100; // AmbientのチャネルID
const char* writeKey = "writeKey"; // ライトキー

BLEScan* pBLEScan;

void setup() {
#ifdef ARDUINO_M5Stack_Core_ESP32
    M5.begin();
    dacWrite(25, 0); // Speaker OFF
#endif

    Serial.begin(115200);
    Serial.print("\r\nscan start.\r\n");

    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(false);

    WiFi.begin(ssid, password);  //  Wi-Fi APに接続
    while (WiFi.status() != WL_CONNECTED) {  //  Wi-Fi AP接続待ち
        Serial.print(".");
        delay(100);
    }
    Serial.print("\r\nWiFi connected\r\nIP address: ");
    Serial.println(WiFi.localIP());

    ambient.begin(channelId, writeKey, &client); // チャネルIDとライトキーを指定してAmbientの初期化
}

void loop() {
    bool found = false;
    float temp, humid, press;

    BLEScanResults foundDevices = pBLEScan->start(3);
    int count = foundDevices.getCount();
    for (int i = 0; i < count; i++) {
        BLEAdvertisedDevice d = foundDevices.getDevice(i);
        if (d.haveManufacturerData()) {
            std::string data = d.getManufacturerData();
            int manu = data[1] << 8 | data[0];
            if (manu == MyManufacturerId && seq != data[2]) {
                found = true;
                seq = data[2];
                temp = (float)(data[4] << 8 | data[3]) / 100.0;
                humid = (float)(data[6] << 8 | data[5]) / 100.0;
                press = (float)(data[8] << 8 | data[7]) * 10.0 / 100.0;
                Serial.printf(">>> seq: %d, t: %.1f, h: %.1f, p: %.1f\r\n", seq, temp, humid, press);
            }
        }
    }

    if (found) {
        // 温度、湿度、気圧、CO2、TVOCの値をAmbientに送信する
        ambient.set(1, temp);
        ambient.set(2, humid);
        ambient.set(3, press);
        ambient.send();
    }
}

