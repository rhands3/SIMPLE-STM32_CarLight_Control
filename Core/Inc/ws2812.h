/*
 * ws2812.h
 *
 *  Created on: Oct 11, 2025
 *      Author: 86151
 */

#ifndef INC_WS2812_H_
#define INC_WS2812_H_
#define LED_COUNT 10
#include "tim.h"

//呼吸
typedef enum {
    WS2812_EFFECT_OFF = 0,
    WS2812_EFFECT_BREATHING,
    WS2812_EFFECT_STEADY
} WS2812_Effect_t;

//颜色变换
typedef enum {
    COLOR_PURPLE_BLUE = 0,  // 蓝紫色
    COLOR_RED,              // 红色
    COLOR_GREEN,            // 绿色
    COLOR_BLUE,             // 蓝色
    COLOR_YELLOW,           // 黄色
    COLOR_CYAN,             // 青色
    COLOR_WHITE,            // 白色
    COLOR_COUNT             // 颜色总数
} BreathColor_t;

//转向灯、双闪
typedef enum {
    TURN_OFF = 0,      // 关闭
    TURN_LEFT,         // 左转向
    TURN_RIGHT,        // 右转向
    TURN_HAZARD        // 双闪警示
} TurnSignal_t;

void ws2812_Update();
void ws2812_Set(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void ws2812_SetALL(uint8_t r, uint8_t g, uint8_t b);
void ws2812_Start();
void ws2812_Off();
void ws2812_StartBreathing();
void ws2812_StopBreathing();
void ws2812_SetSteadyColor(uint8_t r, uint8_t g, uint8_t b);
void ws2812_UpdateBreathing();
void ws2812_StartTurnSignal(TurnSignal_t direction);
void ws2812_StopTurnSignal();
void ws2812_UpdateTurnSignal();
void ws2812_UpdateLeftTurnSignal(uint8_t r, uint8_t g, uint8_t b);
void ws2812_UpdateRightTurnSignal(uint8_t r, uint8_t g, uint8_t b);
void ws2812_UpdateHazardSignal(uint8_t r, uint8_t g, uint8_t b);
#endif /* INC_WS2812_H_ */
