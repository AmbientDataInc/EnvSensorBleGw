# EnvSensorBleGw

環境センサーで測定した温度、湿度などのデーターをBluetooth Low Energy (BLE) でゲートウェイに取り込み、[IoTデーター可視化サービス「Ambient」](https://ambidata.io)に送信するシステム。

![全体の構成](./images/structure.jpg)

* src/envSensor_esp32/BME280_test/BME280_test.ino: M5Stack/ESPr Developer 32とBME280の動作確認プログラム
* src/envSensor_esp32/BLE_BME280/BLE_BME280.ino: M5Stack/ESPr Developer 32のBLE環境センサー端末(コネクトモード)
* src/envSensor_esp32/BLE_BME280_bcast/BLE_BME280_bcast.ino: M5Stack/ESPr Developer 32のBLE環境センサー端末(ブロードキャストモード)
* src/env2ambientCS.py: 環境センサー「2JCIE-BL01」をスキャン、コネクトして温度、湿度、気圧、照度、UV、騒音、電池電圧データーを取得し、Ambientに送信する。
* src/env2ambientBS.py: 環境センサー「2JCIE-BL01」をブロードキャストモードに設定し、アドバタイズデーターに載せた温度、湿度、気圧、照度、UV、騒音、電池電圧データーを取得してAmbientに送信する。
