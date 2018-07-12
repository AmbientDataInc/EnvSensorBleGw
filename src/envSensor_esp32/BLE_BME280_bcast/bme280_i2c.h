#ifndef BME280_i2c_H
#define BME280_i2c_H

#include <Wire.h>
#include "bme280.h"

class BME280
{
public:
    BME280(uint8_t);
    virtual ~BME280();

    void begin(void);
    int8_t get_sensor_data(struct bme280_data *);
private:
    struct bme280_dev dev;
};

#endif // BME280_i2c_H

