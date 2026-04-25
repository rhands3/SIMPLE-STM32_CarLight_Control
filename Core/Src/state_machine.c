#include "state_machine.h"
#include "ws2812.h"
#include "oled.h"
#include <stdio.h>
#include "main.h"
#include "Sensor.h"
#include "tim.h"
#include "bh1750.h"
//key1 自动大灯模式
//key2 自动远近光模式
// 状态变量
SystemState_t current_state = STATE_INIT;
uint32_t state_entry_time = 0;
uint8_t state_changed = 0;
static AnimationState_t animation_state = ANIMATION_SCROLL_NAME;
static uint32_t animation_start_time = 0;
static uint8_t scroll_position = 0;
static uint8_t animation_played = 0;

void StateMachine_Init(void)
{
    // 初始状态为初始化模式
    current_state = STATE_INIT;
    state_entry_time = HAL_GetTick();
    state_changed = 1;  // 标记状态刚改变

    // 初始化LED为RED色


}

//按键检测逻辑
void StateMachine_HandleButtons(void)
{
	static GPIO_PinState last_key1 = GPIO_PIN_SET;
	static GPIO_PinState last_key2 = GPIO_PIN_SET;
	static GPIO_PinState last_key3 = GPIO_PIN_SET;
	static uint32_t last_key_time = 0;
    GPIO_PinState current_key1 = HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin);
    GPIO_PinState current_key2 = HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin);
    GPIO_PinState current_key3 = HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin);
    static uint8_t combo_pending = 0;
    static uint32_t combo_start_time = 0;
    if (!combo_pending &&
           last_key1 == GPIO_PIN_SET && last_key2 == GPIO_PIN_SET &&
           current_key1 == GPIO_PIN_RESET && current_key2 == GPIO_PIN_RESET) {
		   HAL_Delay(10);
			if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET &&
				HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET) {
				combo_pending = 1;
				combo_start_time = HAL_GetTick();
				}
       }
    if (combo_pending) {
        uint8_t key1_still_pressed = (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET);
        uint8_t key2_still_pressed = (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET);
        if (key1_still_pressed && key2_still_pressed) {
            if (HAL_GetTick() - combo_start_time > 80) {
                StateMachine_SwitchToState(STATE_HAZARD);
                combo_pending = 0;
                last_key_time = HAL_GetTick();
                uint32_t release_start = HAL_GetTick();
                while ((HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET ||
                       HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET) &&
                       (HAL_GetTick() - release_start < 300)) {
                    HAL_Delay(10);
                }
                last_key1 = GPIO_PIN_SET;
                last_key2 = GPIO_PIN_SET;
                last_key3 = current_key3;
                return;
            }
        } else {
            combo_pending = 0;
        }
    }
    if (!combo_pending){
    if (last_key1 == GPIO_PIN_SET && current_key1 == GPIO_PIN_RESET) {
    	HAL_GPIO_WritePin(LED_Green_GPIO_Port, LED_Green_Pin, GPIO_PIN_RESET);
    	HAL_GPIO_WritePin(LED_Blue_GPIO_Port, LED_Blue_Pin, GPIO_PIN_RESET);
    	HAL_GPIO_WritePin(LED_Red_GPIO_Port, LED_Red_Pin, GPIO_PIN_RESET);
        if (current_state == STATE_MANUAL_AHB || current_state == STATE_HIGHLIGHT) {
        	StateMachine_SwitchToState(STATE_LOWLIGHT);
        }
        else if(current_state == STATE_CORNER_LIGHT ||current_state == STATE_TURN_LEFT){
        	StateMachine_SwitchToState(STATE_TURN_RIGHT);
        }
		else if(current_state == STATE_LOWLIGHT || current_state == STATE_TURN_RIGHT || current_state == STATE_HAZARD ||
				current_state == STATE_AUTO_HEADLIGHT){
			StateMachine_SwitchToState(STATE_INIT);
		}
        else {
            StateMachine_SwitchToState(STATE_AUTO_HEADLIGHT);
        }
        HAL_Delay(100);
    }
    if (last_key2 == GPIO_PIN_SET && current_key2 == GPIO_PIN_RESET) {
    	HAL_GPIO_WritePin(LED_Green_GPIO_Port, LED_Green_Pin, GPIO_PIN_RESET);
    	HAL_GPIO_WritePin(LED_Blue_GPIO_Port, LED_Blue_Pin, GPIO_PIN_RESET);
    	HAL_GPIO_WritePin(LED_Red_GPIO_Port, LED_Red_Pin, GPIO_PIN_RESET);
    	if (current_state == STATE_AHB) {
		StateMachine_SwitchToState(STATE_MANUAL_AHB);    	}
    	else if (current_state == STATE_MANUAL_AHB || current_state == STATE_LOWLIGHT) {
    		StateMachine_SwitchToState(STATE_HIGHLIGHT);
		}
        else if(current_state == STATE_CORNER_LIGHT || current_state == STATE_TURN_RIGHT){
        	StateMachine_SwitchToState(STATE_TURN_LEFT);
        }
		else if(current_state == STATE_HIGHLIGHT || current_state == STATE_TURN_LEFT || current_state == STATE_HAZARD){
			StateMachine_SwitchToState(STATE_INIT);
		}
    	else{
		StateMachine_SwitchToState(STATE_AHB);
		  }
        HAL_Delay(100);
    }
}
	if (last_key3 == GPIO_PIN_SET && current_key3 == GPIO_PIN_RESET) {
		HAL_GPIO_WritePin(LED_Green_GPIO_Port, LED_Green_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LED_Blue_GPIO_Port, LED_Blue_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LED_Red_GPIO_Port, LED_Red_Pin, GPIO_PIN_RESET);
		if (current_state == STATE_MANUAL_AHB ||
		    current_state == STATE_LOWLIGHT ||
			current_state == STATE_HIGHLIGHT ||
			current_state == STATE_TURN_LEFT||
			current_state == STATE_TURN_RIGHT||
			current_state == STATE_HAZARD ||
			current_state == STATE_AHB ||
			current_state == STATE_AUTO_HEADLIGHT) {
		            StateMachine_SwitchToState(STATE_INIT);
		        } else if(current_state == STATE_CORNER_LIGHT){
		        	StateMachine_SwitchToState(STATE_INIT);

		        }
		        else if (current_state == STATE_INIT){
		        	StateMachine_SwitchToState(STATE_CORNER_LIGHT);
		        }
		        HAL_Delay(100);
	}
    last_key1 = current_key1;
    last_key2 = current_key2;
    last_key3 = current_key3;
}

//切换状态
void StateMachine_SwitchToState(SystemState_t new_state)
{
    if (new_state != current_state) {
        current_state = new_state;
        state_entry_time = HAL_GetTick();
        state_changed = 1;
    }
    else{
    	OLED_NewFrame();
    	OLED_PrintString( 10, 10, "Update...", &font16x16 , OLED_COLOR_NORMAL);
    	OLED_PrintString( 10, 30, "Plz press other key", &font8x8 , OLED_COLOR_NORMAL);
    	OLED_ShowFrame();
    }
}
void StateMachine_Update(void)
{
	 static uint32_t last_state_check = 0;
	    if (HAL_GetTick() - last_state_check > 1000) {
	        if (state_changed == 1 && HAL_GetTick() - state_entry_time > 5000) {
	            state_changed = 0;
	        }
	        last_state_check = HAL_GetTick();
	    }
    switch (current_state){
		case STATE_INIT:
			if (state_changed == 1) {
				 ws2812_StartBreathing();
				 state_changed = 0;
			}
			if (!animation_played) {
			        oled_start_init_2();if (animation_state == ANIMATION_COMPLETE) {
			            animation_played = 1;
			        }
			    } else {
			        OLED_NewFrame();
			        OLED_PrintString(5, 5, " ==车载大灯操作设置==", &font12x12, OLED_COLOR_NORMAL);
			        OLED_PrintString(5, 16, "KEY1:自动大灯", &font12x12, OLED_COLOR_NORMAL);
			        OLED_PrintString(5, 27, "KEY2:自动远近光", &font12x12, OLED_COLOR_NORMAL);
			        OLED_PrintString(5, 38, "EC11:转向辅助灯", &font12x12, OLED_COLOR_NORMAL);
			        OLED_PrintString(5, 50, "K1+K2:双闪警告", &font12x12, OLED_COLOR_NORMAL);
			        OLED_ShowFrame();
			    }
			ws2812_UpdateBreathing();
			break;
		case STATE_AUTO_HEADLIGHT:
		    if (state_changed == 1) {
		        OLED_NewFrame();
		        OLED_PrintString( 10, 10, "Mode:自动大灯", &font16x16 , OLED_COLOR_NORMAL);
		        OLED_PrintString( 10, 25, "EC11:退出", &font16x16 , OLED_COLOR_NORMAL);
		        OLED_ShowFrame();
		        ws2812_StopBreathing();
		        state_changed = 0;
		    }
		    BH1750_Update();
		    HAL_Delay(10);
			break;

		case STATE_AHB:
			if (state_changed == 1) {

				OLED_NewFrame();
				OLED_PrintString( 8, 5, "Mode:自动远近光", &font12x12 , OLED_COLOR_NORMAL);
				OLED_PrintString( 8, 35, "按下K2进入手动模式", &font12x12 , OLED_COLOR_NORMAL);
				OLED_PrintString( 8, 50, "EC11:退出", &font12x12 , OLED_COLOR_NORMAL);
				OLED_ShowFrame();
		        ws2812_StopBreathing();
				state_changed = 0;
			}
			AutoLight_Update();

			break;
		case STATE_CORNER_LIGHT:
			if (state_changed == 1) {

				OLED_NewFrame();
				OLED_PrintString(10, 5, "Mode:转向灯模式", &font12x12 , OLED_COLOR_NORMAL);
				OLED_PrintString(10, 16, "KEY1:右转向灯", &font12x12, OLED_COLOR_NORMAL);
				OLED_PrintString(10, 27, "KEY2:左转向灯", &font12x12, OLED_COLOR_NORMAL);
				OLED_PrintString(10, 38, "EC11:退出", &font12x12, OLED_COLOR_NORMAL);
				OLED_PrintString(10, 49, "K1+K2:双闪警告", &font12x12, OLED_COLOR_NORMAL);
				OLED_ShowFrame();
				 ws2812_StopBreathing();
				state_changed = 0;
			}

			break;
		 case STATE_TURN_LEFT:
			if (state_changed == 1) {
				OLED_NewFrame();
				OLED_PrintString(10, 8, "Mode:左转向灯", &font16x16, OLED_COLOR_NORMAL);
				OLED_PrintString(10, 28, "KEY1:右转向灯", &font16x16, OLED_COLOR_NORMAL);
				OLED_PrintString(10, 48, "EC11:退出/再按一次退出", &font12x12, OLED_COLOR_NORMAL);
				OLED_ShowFrame();
				ws2812_StartTurnSignal(TURN_LEFT);
				state_changed = 0;
			}
			ws2812_UpdateTurnSignal();
			break;
		 case STATE_TURN_RIGHT:
			 if (state_changed == 1) {
				 OLED_NewFrame();
				 OLED_PrintString(10, 8, "Mode:右转向灯", &font16x16, OLED_COLOR_NORMAL);
				 OLED_PrintString(10, 28, "KEY2:左转向灯", &font16x16, OLED_COLOR_NORMAL);
				 OLED_PrintString(10, 48, "EC11:退出/再按一次退出", &font12x12, OLED_COLOR_NORMAL);
				 OLED_ShowFrame();
				 ws2812_StartTurnSignal(TURN_RIGHT);
				 state_changed = 0;
			 }
			 ws2812_UpdateTurnSignal();
			 break;

		 case STATE_HAZARD:
			 if (state_changed == 1) {
				 OLED_NewFrame();
				 OLED_PrintString(10, 10, "Mode:双闪警示灯", &font16x16, OLED_COLOR_NORMAL);
				 OLED_PrintString(10, 30, "EC11:退出", &font12x12, OLED_COLOR_NORMAL);
				 OLED_PrintString(10, 40, "KEY1/2:退出", &font12x12, OLED_COLOR_NORMAL);
				 OLED_ShowFrame();
				 ws2812_StartTurnSignal(TURN_HAZARD);
				 state_changed = 0;
			 }
			 ws2812_UpdateTurnSignal();
			 break;
		 case STATE_MANUAL_AHB:
				 if (state_changed == 1) {
					 OLED_NewFrame();
					 OLED_PrintString(10, 5, "Mode:手动远近光", &font12x12, OLED_COLOR_NORMAL);
					 OLED_PrintString(10, 20, "KEY1:近光", &font12x12, OLED_COLOR_NORMAL);
					 OLED_PrintString(10, 35, "KEY2:远光", &font12x12, OLED_COLOR_NORMAL);
					 OLED_PrintString(10, 50, "EC11:退出", &font12x12, OLED_COLOR_NORMAL);
					 OLED_ShowFrame();
					 ws2812_StopBreathing();
					 state_changed = 0;
				 }

				 break;
		 case STATE_LOWLIGHT:
				 if (state_changed == 1) {
					 OLED_NewFrame();
					 OLED_PrintString(10, 5, "Mode:近光", &font12x12, OLED_COLOR_NORMAL);
					 OLED_PrintString(10, 18, "KEY1:退出", &font12x12, OLED_COLOR_NORMAL);
					 OLED_PrintString(10, 32, "KEY2:远光", &font12x12, OLED_COLOR_NORMAL);
					 OLED_PrintString(10, 48, "EC11:退出", &font12x12, OLED_COLOR_NORMAL);
					 OLED_ShowFrame();
					 state_changed = 0;
				 }
				 ws2812_SetSteadyColor(50, 50, 50);
				 break;
		 case STATE_HIGHLIGHT:
				 if (state_changed == 1) {
					 OLED_NewFrame();
					 OLED_PrintString(10, 5, "Mode:远光", &font12x12, OLED_COLOR_NORMAL);
					 OLED_PrintString(10, 18, "KEY1:近光", &font12x12, OLED_COLOR_NORMAL);
					 OLED_PrintString(10, 32, "KEY2:退出", &font12x12, OLED_COLOR_NORMAL);
					 OLED_PrintString(10, 48, "EC11:退出", &font12x12, OLED_COLOR_NORMAL);
					 OLED_ShowFrame();
					 state_changed = 0;
				 }
				 ws2812_SetSteadyColor(255, 255, 255);
				 break;
    }
}


void oled_start_init_2(void)
{
	if (animation_state == ANIMATION_COMPLETE) {
	        return;
	    }
    uint32_t current_time = HAL_GetTick();

    switch (animation_state) {
        case ANIMATION_SCROLL_NAME:
            if (current_time - animation_start_time > 5) {
                OLED_NewFrame();
                OLED_PrintString(scroll_position - 100, 10, "2301 陈宇杰", &font16x16, OLED_COLOR_NORMAL);
                OLED_PrintString(scroll_position - 120, 30, "====车载大灯====", &font16x16, OLED_COLOR_NORMAL);
                OLED_ShowFrame();

                scroll_position++;
                animation_start_time = current_time;

                if (scroll_position >= 120) {
                    animation_state = ANIMATION_SCROLL_MENU;
                    scroll_position = 0;
                    HAL_Delay(1000);
                }
            }
            break;

        case ANIMATION_SCROLL_MENU:
            if (current_time - animation_start_time > 10) {
                OLED_NewFrame();
                OLED_PrintString(5, 5, " ==车载大灯操作设置==", &font12x12, OLED_COLOR_NORMAL);
                OLED_PrintString(5, 16, "KEY1:自动大灯", &font12x12, OLED_COLOR_NORMAL);
                OLED_PrintString(5, 27, "KEY2:自动远近光", &font12x12, OLED_COLOR_NORMAL);
                OLED_PrintString(5, 38, "EC11:转向辅助灯", &font12x12, OLED_COLOR_NORMAL);
                OLED_PrintString(5, 50, "K1+K2:双闪警告", &font12x12, OLED_COLOR_NORMAL);
                OLED_ShowFrame();

                scroll_position++;
                animation_start_time = current_time;

                if (scroll_position >= 238) {
                    animation_state = ANIMATION_COMPLETE;
                }
            }
            break;

        case ANIMATION_COMPLETE:

            break;
    }
}

void reset_animation(void)
{
    animation_state = ANIMATION_SCROLL_NAME;
    scroll_position = 0;
    animation_start_time = HAL_GetTick();
}










