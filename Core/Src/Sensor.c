//	  upEdge = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);
//	  downEdge = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_4);
//	  distance = (downEdge - upEdge) * 0.034 / 2;

//float Ultrasonic_Start(float distance){
//	HAL_GPIO_WritePin(Trig_GPIO_Port, Trig_Pin, GPIO_PIN_SET);
//	HAL_Delay(1);
//	HAL_GPIO_WritePin(Trig_GPIO_Port, Trig_Pin, GPIO_PIN_RESET);
//	__HAL_TIM_SET_COUNTER(&htim1,0);
//	HAL_Delay(20);
//	sprintf(message, "distance: %.2f",distance);
//	OLED_PrintString( 10, 30, message, &font16x16 , OLED_COLOR_NORMAL);
//	OLED_ShowFrame();
//	return distance;
//}


#include "main.h"
#include <stdio.h>
#include "Sensor.h"
#include "ws2812.h"
#include "oled.h"
#include "string.h"
#include "tim.h"

// 超声波模块相关变量
static LightMode_t current_light_mode = LIGHT_OFF;

volatile int upEdge = 0;
volatile int downEdge = 0;
volatile float distance = 0;
volatile char message [20] = "";
volatile uint8_t new_distance_available = 0;
volatile uint8_t measurement_complete  = 0;   //等待上下升岩
// 远近光颜色定义
static const uint8_t low_beam_color[3] = {50, 50, 50};    // 近光：白色，亮度较低
static const uint8_t high_beam_color[3] = {255, 255, 255};   // 远光：白色，亮度最高

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	if (htim == &htim1)
	  {
	    // 通道3中断：上升沿捕获（回声开始）
	    if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
	    {
	      upEdge = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);
	    }
	    // 通道4中断：下降沿捕获（回声结束）
	    if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
	    {
	      downEdge = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_4);


	      // 计算脉冲宽度
	      uint32_t pulse_width;
	      if (downEdge > upEdge) {
	        pulse_width = downEdge - upEdge;
	      } else {
	        pulse_width = (0xFFFF - upEdge) + downEdge;
	      }
	      // 计算距离
	      distance = (float)pulse_width * 0.034f / 2.0f;
	      new_distance_available = 1;
	      measurement_complete  = 1;

	      // 停止捕获，等待下一次触发
	      HAL_TIM_IC_Stop_IT(&htim1, TIM_CHANNEL_3);
	      HAL_TIM_IC_Stop_IT(&htim1, TIM_CHANNEL_4);
	    }
	  }
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if(htim->Instance == TIM2) {  // 作为测量定时器
        static uint8_t trigger_state = 0;
        static uint32_t trigger_start_time = 0;

        switch(trigger_state) {
            case 0:  // 发送触发脉冲
                HAL_GPIO_WritePin(Trig_GPIO_Port, Trig_Pin, GPIO_PIN_SET);
                trigger_start_time = HAL_GetTick();
                trigger_state = 1;
                break;

            case 1:  // 结束触发脉冲
                if(HAL_GetTick() - trigger_start_time >= 1) {
                    HAL_GPIO_WritePin(Trig_GPIO_Port, Trig_Pin, GPIO_PIN_RESET);
                    upEdge = 0;
                    downEdge = 0;
                    measurement_complete  = 0;  // 标记可以计算距离
                    trigger_state = 0;

                    __HAL_TIM_SET_COUNTER(&htim1, 0);
                    HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_3);  // 上升沿捕获
					HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_4);  // 下降沿捕获
					trigger_state = 0;
                }
                break;
        }
    }
}

//// 设置灯光模式
void SetLightMode(LightMode_t mode)
{
    if (current_light_mode == mode) {
        return;  // 模式未改变，不需要更新
    }

    current_light_mode = mode;

    switch (mode) {
        case LIGHT_OFF:
            // 关闭所有灯光
            ws2812_StopBreathing();
            ws2812_SetALL(0, 0, 0);

            break;

        case LOW_BEAM:
            // 设置近光模式
            ws2812_StopBreathing();
            ws2812_SetALL(0, 0, 0);
            ws2812_SetSteadyColor(low_beam_color[0], low_beam_color[1], low_beam_color[2]);

            break;

        case HIGH_BEAM:
            // 设置远光模式
            ws2812_StopBreathing();
            ws2812_SetALL(0, 0, 0);
            ws2812_SetSteadyColor(high_beam_color[0], high_beam_color[1], high_beam_color[2]);
            break;
        case WARNING_MODE:
            ws2812_StopBreathing();
            ws2812_SetALL(0, 0, 0);
        	break;
    }
}

void AutoLight_Update(void)
{
	float min_distance = 5.0;
	float max_distance = 50.0;
	if(new_distance_available == 1){
		new_distance_available = 0;
	float current_distance = distance;
    char message[20];
    sprintf(message, "距离: %.2f cm", distance);
    OLED_PrintString(8, 20, message, &font12x12, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
	if (current_distance >= min_distance && current_distance <= max_distance) {
			SetLightMode(LOW_BEAM);
	    } else if(current_distance > max_distance){
	    	SetLightMode(HIGH_BEAM);
	    }else if(current_distance < min_distance){
	    	SetLightMode(WARNING_MODE);
	    }
	}
	static uint32_t last_warning_time = 0;
	static uint8_t warning_flash = 0;

	    if (current_light_mode == WARNING_MODE) {
	        uint32_t current_time = HAL_GetTick();
	        if (current_time - last_warning_time > 200) { // 200ms闪烁间隔
	            if (warning_flash) {
	                ws2812_SetALL(255, 0, 0);  // 红色
	            } else {
	                ws2812_SetALL(0, 0, 0);    // 熄灭
	            }
	            warning_flash = !warning_flash;
	            last_warning_time = current_time;
	            ws2812_Update();
	        }
	    }
}
//    // 检查是否到达测量间隔
//    if (current_time - last_measure_time < MEASURE_INTERVAL) {
//        return;
//    }
//
//    last_measure_time = current_time;
//
//    // 获取当前距离
//    current_distance = Ultrasonic_GetDistance();
//
//    // 根据距离自动切换远近光
//    if (current_distance > HIGH_BEAM_DISTANCE) {
//        // 前方无障碍物或障碍物较远，开启远光
//        SetLightMode(HIGH_BEAM);
//    }
//    else if (current_distance > LOW_BEAM_DISTANCE && current_distance <= HIGH_BEAM_DISTANCE) {
//        // 检测到中距离障碍物，切换为近光
//        SetLightMode(LOW_BEAM);
//    }
//    else if (current_distance <= LOW_BEAM_DISTANCE) {
//        // 检测到近距离障碍物，可以进一步降低亮度或闪烁警告
//        SetLightMode(LOW_BEAM);
//
//        // 添加闪烁警告效果
//        static uint8_t warning_flash = 0;
//        if (warning_flash) {
//            ws2812_SetALL(255, 0, 0);  // 红色警告
//        } else {
//            ws2812_SetALL(low_beam_color[0], low_beam_color[1], low_beam_color[2]);
//        }
//        warning_flash = !warning_flash;
//        ws2812_Update();
//    }
//}

//uint32_t Ultrasonic_GetDistance(void)
//{
//    uint32_t distance = 0;
//    uint32_t pulse_width = 0;
//
//    // 发送10us的触发信号
//    HAL_GPIO_WritePin(Trig_GPIO_Port, Trig_Pin, GPIO_PIN_SET);
//    HAL_Delay(1);  // 延时10us (实际为1ms，需要根据实际情况调整)
//    HAL_GPIO_WritePin(Trig_GPIO_Port, Trig_Pin, GPIO_PIN_RESET);
//
//    // 等待Echo引脚变高
//    while (HAL_GPIO_ReadPin(Echo_GPIO_Port, Echo_Pin) == GPIO_PIN_RESET);
//
//    // 记录高电平开始时间
//    uint32_t start_time = HAL_GetTick();
//
//    // 等待Echo引脚变低
//    while (HAL_GPIO_ReadPin(Echo_GPIO_Port, Echo_Pin) == GPIO_PIN_SET);
//
//    // 记录高电平结束时间
//    uint32_t end_time = HAL_GetTick();
//
//    // 计算高电平持续时间（脉冲宽度）
//    pulse_width = end_time - start_time;
//
//    // 计算距离（声音速度340m/s，即0.034cm/us）
//    // 距离 = (脉冲宽度 * 声速) / 2
//    distance = (pulse_width * 34) / 2000;  // 转换为厘米
//
//    // 距离限制在有效范围内
//    if (distance < ULTRASONIC_MIN_DISTANCE) {
//        distance = ULTRASONIC_MIN_DISTANCE;
//    } else if (distance > ULTRASONIC_MAX_DISTANCE) {
//        distance = ULTRASONIC_MAX_DISTANCE;
//    }
//
//    return distance;
//}



//}
//
//LightMode_t GetCurrentLightMode(void)
//{
//    return current_light_mode;
//}
//

