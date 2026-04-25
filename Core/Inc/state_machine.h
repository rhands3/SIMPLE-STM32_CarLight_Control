/*
 * state_machine.h
 *
 *  Created on: Oct 13, 2025
 *      Author: 陈宇杰
 */

#ifndef INC_STATE_MACHINE_H_
#define INC_STATE_MACHINE_H_



#include "main.h"

// 系统状态定义
typedef enum {
    STATE_INIT = 0,         // 初始化状态
    STATE_AUTO_HEADLIGHT,   // 自动大灯模式
    STATE_AHB,              // 自动远近光模式
	STATE_TURN_LEFT,      // 左转向
	STATE_TURN_RIGHT,     // 右转向
	STATE_HAZARD,          // 双闪警示
	STATE_CORNER_LIGHT,		//转向灯辅助模式
	STATE_MANUAL_AHB,	//手动远近光
	STATE_HIGHLIGHT,
	STATE_LOWLIGHT
} SystemState_t;

typedef enum {
    ANIMATION_SCROLL_NAME = 0,
    ANIMATION_SCROLL_MENU,
    ANIMATION_COMPLETE
} AnimationState_t;

void StateMachine_Init(void);
void StateMachine_Update(void);
void StateMachine_HandleButtons(void);
void StateMachine_SwitchToState(SystemState_t new_state);
void oled_start_init_2(void);
void reset_animation(void);


// 外部变量
extern SystemState_t current_state;
extern uint32_t state_entry_time;
extern uint8_t state_changed;


#endif /* INC_STATE_MACHINE_H_ */







