/*
 * bh1750.h
 *
 *  Created on: Oct 16, 2025
 *      Author: 86151
 */

#ifndef INC_BH1750_H_
#define INC_BH1750_H_

#include "main.h"
#include "i2c.h"
#include "stm32f1xx_hal.h"

typedef enum {
    BH1750_STATE_READY,
    BH1750_STATE_BUSY,
    BH1750_STATE_ERROR
} BH1750_StateTypeDef;

typedef enum {
    OFF_MODE = 0,      //
    BRIGHT_MODE,           //
    SPRINKLE_MODE,
	DARK_MODE//
} AutolightMode_t;

// 传感器结构体
typedef struct {
    I2C_HandleTypeDef *hi2c;
    BH1750_StateTypeDef state;
    uint32_t last_operation_time;
    uint8_t rx_buffer[2];
    uint16_t raw_data;
    float light_intensity;
} BH1750_HandleTypeDef;

extern BH1750_HandleTypeDef bh1750_sensor;

#define BH1750_ADDR 0x23  // ADDR引脚接GND时的地址
#define BH1750_POWER_DOWN 0x00
#define BH1750_POWER_ON 0x01
#define BH1750_RESET 0x07
#define BH1750_CONTINUOUS_H_RES_MODE 0x10
#define BH1750_CONTINUOUS_H_RES_MODE_2 0x11
#define BH1750_CONTINUOUS_L_RES_MODE 0x13
#define BH1750_ONE_TIME_H_RES_MODE 0x20
#define BH1750_ONE_TIME_H_RES_MODE_2 0x21
#define BH1750_ONE_TIME_L_RES_MODE 0x23

void BH1750_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef BH1750_StartMeasurement(void);
void BH1750_Update();
void SetAutoLightMode(AutolightMode_t mode);

#endif /* INC_BH1750_H_ */
