#include "bh1750.h"
#include "oled.h"
#include <stdio.h>
#include "ws2812.h"
#include "tim.h"


BH1750_HandleTypeDef bh1750_sensor;
static AutolightMode_t current_light_mode = OFF_MODE;

void BH1750_Init(I2C_HandleTypeDef *hi2c){

	bh1750_sensor.hi2c = &hi2c2;
	bh1750_sensor.state = BH1750_STATE_READY;
	bh1750_sensor.light_intensity = 0.0f;
	bh1750_sensor.raw_data = 0;

	uint8_t data;

	    // 开启传感器电源
	    data = BH1750_POWER_ON;
	    HAL_I2C_Master_Transmit(hi2c, BH1750_ADDR << 1, &data, 1, 10);
	    HAL_Delay(10);

	    // 复位传感器
	    data = BH1750_RESET;
	    HAL_I2C_Master_Transmit(hi2c, BH1750_ADDR << 1, &data, 1, 10);
	    HAL_Delay(10);

	    // 设置测量模式为连续高分辨率模式
	    data = BH1750_CONTINUOUS_H_RES_MODE;
	    HAL_I2C_Master_Transmit(hi2c, BH1750_ADDR << 1, &data, 1, 10);

	    bh1750_sensor.state = BH1750_STATE_READY;
}

//测量函数
HAL_StatusTypeDef BH1750_StartMeasurement(void)
{
    // 检查传感器是否就绪，避免重复启动
    if (bh1750_sensor.state != BH1750_STATE_READY) {
        return HAL_BUSY;  // 如果忙碌，返回忙碌状态
    }

    // 设置状态为忙碌，表示测量正在进行
    bh1750_sensor.state = BH1750_STATE_BUSY;

    // 启动非阻塞I2C接收（关键改进！）
    // 函数立即返回，不会阻塞主循环
    HAL_StatusTypeDef status = HAL_I2C_Master_Receive_IT(
		bh1750_sensor.hi2c,           // 使用的I2C接口
        BH1750_ADDR << 1,        // 传感器地址（左移1位是I2C协议要求）
		bh1750_sensor.rx_buffer,      // 数据存储缓冲区
        2                        // 读取2个字节
    );

    // 检查启动是否成功
    if (status != HAL_OK) {
    	bh1750_sensor.state = BH1750_STATE_ERROR;  // 失败则设为错误状态
    }

    return status;  // 返回启动结果
}

//中断回调函数
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    // 检查是否是我们的BH1750传感器完成的传输
    if (hi2c->Instance == bh1750_sensor.hi2c->Instance) {
        // 1. 合并两个字节的原始数据
        //    BH1750返回的数据是16位，分两个字节传输
    	bh1750_sensor.raw_data = (bh1750_sensor.rx_buffer[0] << 8) | bh1750_sensor.rx_buffer[1];

        // 2. 根据BH1750数据手册公式计算光照强度
        //    原始数据除以1.2得到lux值
        bh1750_sensor.light_intensity = (float)bh1750_sensor.raw_data / 1.2f;

        // 3. 测量完成，状态恢复为就绪，可以开始下一次测量
        bh1750_sensor.state = BH1750_STATE_READY;

        // 4. 记录完成时间（用于超时检测）
        bh1750_sensor.last_operation_time = HAL_GetTick();
    }
}

void SetAutoLightMode(AutolightMode_t mode)
{
    if (current_light_mode == mode) {
        return;  // 模式未改变，不需要更新
    }

    current_light_mode = mode;

    switch (mode) {
        case OFF_MODE:
            // 关闭所有灯光
            ws2812_StopBreathing();
            ws2812_SetALL(0, 0, 0);
			HAL_GPIO_WritePin(LED_Green_GPIO_Port, LED_Green_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_Blue_GPIO_Port, LED_Blue_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_Red_GPIO_Port, LED_Red_Pin, GPIO_PIN_RESET);

            break;

        case BRIGHT_MODE:
            // 设置日照大灯模式
            ws2812_StopBreathing();
            ws2812_SetALL(0, 0, 0);
            HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
            HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
			HAL_GPIO_WritePin(LED_Green_GPIO_Port, LED_Green_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED_Blue_GPIO_Port, LED_Blue_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED_Red_GPIO_Port, LED_Red_Pin, GPIO_PIN_SET);

            break;

        case SPRINKLE_MODE:
            // 近光灯
            ws2812_StopBreathing();
            ws2812_SetALL(0, 0, 0);
            ws2812_SetSteadyColor(50, 50, 50);
			HAL_GPIO_WritePin(LED_Green_GPIO_Port, LED_Green_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_Blue_GPIO_Port, LED_Blue_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(LED_Red_GPIO_Port, LED_Red_Pin, GPIO_PIN_RESET);

            break;
        case DARK_MODE:
			 // 设置日照小灯模式
			 ws2812_StopBreathing();
			 ws2812_SetALL(0, 0, 0);
			HAL_GPIO_WritePin(LED_Green_GPIO_Port, LED_Green_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED_Blue_GPIO_Port, LED_Blue_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(LED_Red_GPIO_Port, LED_Red_Pin, GPIO_PIN_RESET);
			 break;
    }
}

void BH1750_Update(void){
	static uint32_t last_update_time = 0;
	    uint32_t current_time = HAL_GetTick();
	    float dark = 50.0;
	    float bright = 500.0;
	    float sprinkle = 1000.0;
	    // 每500ms更新一次显示，避免刷新过快
	    if (current_time - last_update_time < 500) {
	        return;
	    }

	    // 如果传感器就绪，开始新的测量
	    if (bh1750_sensor.state == BH1750_STATE_READY) {
	        BH1750_StartMeasurement();
	    }

	    // 显示当前的光照强度值
	    char message[32];
	    sprintf(message, "Bright: %.2f Lux", bh1750_sensor.light_intensity);
	    OLED_PrintString(10, 40, message, &font12x12, OLED_COLOR_NORMAL);
	    OLED_ShowFrame();
	    if (bh1750_sensor.light_intensity < dark) {
	    	SetAutoLightMode(SPRINKLE_MODE);
		} else if(bh1750_sensor.light_intensity > sprinkle){
			SetAutoLightMode(OFF_MODE);
		}else if(bh1750_sensor.light_intensity >= dark && bh1750_sensor.light_intensity <= bright){
			SetAutoLightMode(BRIGHT_MODE);
		}else if(bh1750_sensor.light_intensity > bright && bh1750_sensor.light_intensity <= sprinkle){
			SetAutoLightMode(DARK_MODE);
		}
	    last_update_time = current_time;
}





