#include "bme280_i2c.h"
#include <Arduino.h>

void user_delay_ms(uint32_t period)
{
    delay(period);
}

int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
    int8_t rslt = 0;
    int i = 0;
    Wire.beginTransmission(dev_id);
    Wire.write(reg_addr);
    Wire.endTransmission();
    Wire.requestFrom(dev_id, len);
    while(Wire.available()){
        reg_data[i] = Wire.read();
        i++;
    }
    return rslt;
}

int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
    int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

    Wire.beginTransmission(dev_id);
    Wire.write(reg_addr);
    Wire.write(reg_data, len);
    Wire.endTransmission();

    return rslt;
}

BME280::BME280(uint8_t dev_id) {
    dev.dev_id = dev_id;
    dev.intf = BME280_I2C_INTF;
    dev.read = user_i2c_read;
    dev.write = user_i2c_write;
    dev.delay_ms = user_delay_ms;
}

BME280::~BME280() {
}

void BME280::begin(void) {
    int8_t rslt = BME280_OK;
    uint8_t settings_sel;

    bme280_init(&dev);

    /* Recommended mode of operation: Indoor navigation */
    dev.settings.osr_h = BME280_OVERSAMPLING_1X;
    dev.settings.osr_p = BME280_OVERSAMPLING_16X;
    dev.settings.osr_t = BME280_OVERSAMPLING_2X;
    dev.settings.filter = BME280_FILTER_COEFF_16;
    dev.settings.standby_time = BME280_STANDBY_TIME_62_5_MS;

    settings_sel = BME280_OSR_PRESS_SEL;
    settings_sel |= BME280_OSR_TEMP_SEL;
    settings_sel |= BME280_OSR_HUM_SEL;
    settings_sel |= BME280_STANDBY_SEL;
    settings_sel |= BME280_FILTER_SEL;
    rslt = bme280_set_sensor_settings(settings_sel, &dev);
    rslt = bme280_set_sensor_mode(BME280_NORMAL_MODE, &dev);
}

int8_t BME280::get_sensor_data(struct bme280_data *data) {
    int8_t rslt = BME280_OK;

    /* Delay while the sensor completes a measurement */
    dev.delay_ms(70);
    rslt = bme280_get_sensor_data(BME280_ALL, data, &dev);
    dev.delay_ms(1);

    return rslt;
}

