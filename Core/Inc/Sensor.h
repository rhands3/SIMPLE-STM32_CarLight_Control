/*
 * Sensor.h
 *
 *  Created on: Oct 15, 2025
 *      Author: 86151
 */

#ifndef INC_SENSOR_H_
#define INC_SENSOR_H_
#include "main.h"



typedef enum {
    LIGHT_OFF = 0,      // 灯光关闭
    LOW_BEAM,           // 近光
    HIGH_BEAM,			// 远光
	WARNING_MODE
} LightMode_t;

void Ultrasonic_Init();
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);
float Ultrasonic_Start(float distance);
void AutoLight_Update(void);
void SetLightMode(LightMode_t mode);
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif /* INC_SENSOR_H_ */
