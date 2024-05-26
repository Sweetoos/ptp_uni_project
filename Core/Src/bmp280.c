#include "bmp280.h"

// Function to read data from a register
static bool read_register(BMP280_HandleTypedef *dev, uint8_t reg, uint8_t *value, uint16_t len) {
    return HAL_I2C_Mem_Read(dev->i2c, dev->addr << 1, reg, I2C_MEMADD_SIZE_8BIT, value, len, 100) == HAL_OK;
}

// Function to write data to a register
static bool write_register(BMP280_HandleTypedef *dev, uint8_t reg, uint8_t value) {
    return HAL_I2C_Mem_Write(dev->i2c, dev->addr << 1, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 100) == HAL_OK;
}

// Function to read calibration data from the sensor
static bool read_calibration_data(BMP280_HandleTypedef *dev) {
    uint8_t calib[24];
    if (!read_register(dev, 0x88, calib, 24)) {
        return false;
    }

    dev->dig_T1 = (calib[1] << 8) | calib[0];
    dev->dig_T2 = (calib[3] << 8) | calib[2];
    dev->dig_T3 = (calib[5] << 8) | calib[4];

    return true;
}

// Initialize the sensor with default parameters
void bmp280_init_default_params(bmp280_params_t *params) {
    params->mode = BMP280_MODE_NORMAL;
    params->filter = BMP280_FILTER_OFF;
    params->oversampling_pressure = BMP280_SKIPPED;
    params->oversampling_temperature = BMP280_STANDARD;
    params->oversampling_humidity = BMP280_SKIPPED;
    params->standby = BMP280_STANDBY_250;
}

// Initialize the BMP280 sensor
bool bmp280_init(BMP280_HandleTypedef *dev, bmp280_params_t *params) {
    dev->params = *params;

    uint8_t chip_id;
    if (!read_register(dev, 0xD0, &chip_id, 1) || (chip_id != BMP280_CHIP_ID && chip_id != BME280_CHIP_ID)) {
        return false;
    }
    dev->id = chip_id;

    if (!write_register(dev, 0xE0, 0xB6)) {
        return false;
    }
    HAL_Delay(100);

    if (!read_calibration_data(dev)) {
        return false;
    }

    uint8_t config = (params->standby << 5) | (params->filter << 2);
    if (!write_register(dev, 0xF5, config)) {
        return false;
    }

    uint8_t ctrl_meas = (params->oversampling_temperature << 5) | (params->oversampling_pressure << 2) | params->mode;
    if (!write_register(dev, 0xF4, ctrl_meas)) {
        return false;
    }

    return true;
}

// Read the raw temperature data
static bool read_raw_temperature(BMP280_HandleTypedef *dev, int32_t *temperature) {
    uint8_t data[3];
    if (!read_register(dev, 0xFA, data, 3)) {
        return false;
    }
    *temperature = (int32_t)(((uint32_t)(data[0]) << 12) | ((uint32_t)(data[1]) << 4) | ((uint32_t)data[2] >> 4));
    return true;
}

// Compensate the temperature using calibration data
static int32_t compensate_temperature(BMP280_HandleTypedef *dev, int32_t adc_T) {
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)dev->dig_T1 << 1))) * ((int32_t)dev->dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dev->dig_T1)) * ((adc_T >> 4) - ((int32_t)dev->dig_T1))) >> 12) * ((int32_t)dev->dig_T3)) >> 14;
    T = var1 + var2;
    return (T * 5 + 128) >> 8;
}

// Read the temperature in degrees Celsius (times 100)
bool bmp280_read_fixed_temperature(BMP280_HandleTypedef *dev, int32_t *temperature) {
    int32_t raw_temperature;
    if (!read_raw_temperature(dev, &raw_temperature)) {
        return false;
    }
    *temperature = compensate_temperature(dev, raw_temperature);
    return true;
}
