#include "ws2812.h"

#define code0 30
#define code1 60
#define codeReset 0

uint8_t color [LED_COUNT][3];

static WS2812_Effect_t ws2812_effect = WS2812_EFFECT_OFF;
static uint32_t last_breath_update = 0;	//记录上次更新时间
static uint8_t breath_brightness = 0;
static int8_t breath_direction = 1;     // 1 - 变亮 ,-1 - 变暗
static uint8_t target_red = 255;
static uint8_t target_green = 0;
static uint8_t target_blue = 0xE1;

static BreathColor_t current_color = COLOR_PURPLE_BLUE;
static uint32_t color_change_time = 0;
static uint32_t color_change_interval = 5000; // 5秒切换颜色

static TurnSignal_t turn_direction = TURN_OFF;
static uint32_t last_turn_update = 0;
static uint8_t turn_animation_step = 0;
static uint8_t turn_animation_phase = 0;

static const uint8_t color_table[COLOR_COUNT][3] = {
    {255, 0,   0xE1},  // 蓝紫色
    {255, 0,   0},     // 红色
    {0,   255, 0},     // 绿色
    {0,   0,   255},   // 蓝色
    {255, 255, 0},     // 黄色
    {0,   255, 255},   // 青色
    {255, 255, 255}    // 白色
};

void ws2812_Set(uint8_t index, uint8_t r, uint8_t g, uint8_t b){
	color[index][0] = r;
	color[index][1] = g;
	color[index][2] = b;
}

void ws2812_SetALL(uint8_t r, uint8_t g, uint8_t b){
	for (int i = 0; i < LED_COUNT; i++){
		ws2812_Set(i, r, g , b);
	}
}

void ws2812_Update(){ // grb
	static uint16_t date [LED_COUNT * 3 * 8 + 1];
	for( int i = 0; i < LED_COUNT; i++){
		uint8_t r = color[i][0];
		uint8_t g = color[i][1];
		uint8_t b = color[i][2];
		for ( int j = 0 ; j < 8 ; j++){
			date[ 24 * i + j ] = (g & (0x80 >>j) )  ? code1 : code0;
			date[ 24 * i + j + 8 ] = (r &(0x80 >>j))  ? code1 : code0;
			date[ 24 * i + j + 16 ] = (b & (0x80 >>j))  ? code1 : code0;
		}
//			if ( g  & 0x80 >> j != 0 ){
//			date[ 24 * i + j ] = code1;
//			}
//			else{
//				date [ 24 * i + j ] = code0;
//			}
//		}
	}
	date[24 * LED_COUNT] = codeReset;
	HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_1);
	__HAL_TIM_SET_COUNTER(&htim3, 0);
	HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_1, (uint32_t*)date, sizeof(date)/sizeof(uint16_t));
}

// 蓝紫循环灯
void ws2812_Start(){
	  for (uint8_t r = 0 ; r<= 0xFF; r++){

	  ws2812_SetALL(r, 0, 0xE1);
	  ws2812_Update();
	  HAL_Delay(10);
	  }
	  HAL_Delay(1000);
}
//ws2812 关闭
void ws2812_Off(){
	  ws2812_SetALL(0, 0, 0);
	  ws2812_Update();
	  HAL_Delay(1000);
}

//非阻塞
//	---------ws2812 呼吸灯---------
// 启动呼吸灯效果
void ws2812_StartBreathing()
{
    ws2812_effect = WS2812_EFFECT_BREATHING;
    breath_brightness = 0;
    breath_direction = 1;
    last_breath_update = HAL_GetTick();

    // 设置目标颜色（蓝紫色）
    target_red = 255;
    target_green = 0;
    target_blue = 0xE1;
}

// 停止呼吸灯效果
void ws2812_StopBreathing()
{
    ws2812_effect = WS2812_EFFECT_OFF;
    ws2812_SetALL(0, 0, 0);
    ws2812_Update();
}

// 设置常亮颜色
void ws2812_SetSteadyColor(uint8_t r, uint8_t g, uint8_t b)
{
    ws2812_effect = WS2812_EFFECT_STEADY;
    target_red = r;
    target_green = g;
    target_blue = b;
    ws2812_SetALL(r, g, b);
    ws2812_Update();
}

// 更新呼吸灯效果（需要在主循环中定期调用）
void ws2812_UpdateBreathing()
{
    uint32_t current_time = HAL_GetTick();

    if (current_time - color_change_time > color_change_interval) {
            current_color = (current_color + 1) % COLOR_COUNT;
            color_change_time = current_time;
        }

    // 限制更新频率，避免过于频繁  检查是否到了该更新的时候
    if (current_time - last_breath_update < 10) {  // 每30ms更新一次
        return; // 时间还没到，直接返回
    }

    last_breath_update = current_time;   //记录此次更新时间

    switch (ws2812_effect) {
        case WS2812_EFFECT_BREATHING:
            // 更新呼吸亮度
            breath_brightness += breath_direction * 5;  // 每次变化5，调整这个值可以改变呼吸速度

            // 边界检查
            if (breath_brightness >= 255) {
                breath_brightness = 255;
                breath_direction = -1;  // 开始变暗
            } else if (breath_brightness <= 0) {
                breath_brightness = 0;
                breath_direction = 1;   // 开始变亮
            }

            // 使用当前颜色
            uint8_t r = color_table[current_color][0];
		    uint8_t g = color_table[current_color][1];
		    uint8_t b = color_table[current_color][2];
            // 计算当前颜色（根据亮度调整）
            uint8_t current_red = (r * breath_brightness) / 255;
            uint8_t current_green = (g * breath_brightness) / 255;
            uint8_t current_blue = (b * breath_brightness) / 255;

            // 应用颜色并更新
            ws2812_SetALL(current_red, current_green, current_blue);
            ws2812_Update();
            break;

        case WS2812_EFFECT_STEADY:
            // 常亮模式，不需要更新
            break;

        case WS2812_EFFECT_OFF:
            // 关闭模式，不需要更新
            break;
    }
}


//-----------转向灯ws2812-----------
//初始化转向灯
void ws2812_StartTurnSignal(TurnSignal_t direction)
{
    turn_direction = direction;
    turn_animation_step = 0;
    turn_animation_phase = 0;
    last_turn_update = HAL_GetTick();   //依旧老嵌套方法    gettick函数是实时计算机从开始到现在计时

    // 先关闭所有LED
    ws2812_SetALL(0, 0, 0);
    ws2812_Update();
}
//关闭转向灯
void ws2812_StopTurnSignal()
{
    turn_direction = TURN_OFF;
    ws2812_SetALL(0, 0, 0);
    ws2812_Update();
}
//更新转向灯函数
void ws2812_UpdateTurnSignal(void)
{
    if (turn_direction == TURN_OFF) {
        return;
    }

    uint32_t current_time = HAL_GetTick();

    // 每150ms更新一次流水效果
    if (current_time - last_turn_update < 150) {
        return;
    }

    last_turn_update = current_time;

    // 转向灯颜色：黄色 (255, 100, 0)
    uint8_t turn_color_r = 255;
    uint8_t turn_color_g = 100;
    uint8_t turn_color_b = 0;

    switch (turn_direction) {
        case TURN_LEFT:  // 左转向：从右向左流水
            ws2812_UpdateLeftTurnSignal(turn_color_r, turn_color_g, turn_color_b);
            break;

        case TURN_RIGHT: // 右转向：从左向右流水
            ws2812_UpdateRightTurnSignal(turn_color_r, turn_color_g, turn_color_b);
            break;

        case TURN_HAZARD: // 双闪警示：全部闪烁
            ws2812_UpdateHazardSignal(turn_color_r, turn_color_g, turn_color_b);
            break;

        default:
            break;
    }

    ws2812_Update();
}
// 左转向流水效果
void ws2812_UpdateLeftTurnSignal(uint8_t r, uint8_t g, uint8_t b)
{
    // 先关闭所有LED
    ws2812_SetALL(0, 0, 0);

    // 流水效果：从右向左点亮
    switch (turn_animation_step) {
        case 0: // 点亮最右边2个LED
            if (LED_COUNT >= 2) {
                ws2812_Set(4, r, g, b);
                ws2812_Set(5, r, g, b);
            }
            break;

        case 1: // 点亮中间2个LED
            if (LED_COUNT >= 4) {
                ws2812_Set(LED_COUNT-3, r, g, b);
                ws2812_Set(LED_COUNT-4, r, g, b);
            }
            break;

        case 2: // 点亮最左边2个LED
            if (LED_COUNT >= 2) {
                ws2812_Set(LED_COUNT-1, r, g, b);
                ws2812_Set(LED_COUNT-2, r, g, b);
            }
            break;

        case 3: // 全部熄灭
            // 已经在上面的SetALL中关闭了
            break;
    }

    turn_animation_step++;
    if (turn_animation_step >= 4) {
        turn_animation_step = 0;
    }
}

// 右转向流水效果
void ws2812_UpdateRightTurnSignal(uint8_t r, uint8_t g, uint8_t b)
{
    // 先关闭所有LED
    ws2812_SetALL(0, 0, 0);

    // 流水效果：从左向右点亮
    switch (turn_animation_step) {
        case 0: // 点亮最左边2个LED
            if (LED_COUNT >= 2) {
                ws2812_Set(0, r, g, b);
                ws2812_Set(1, r, g, b);
            }
            break;

        case 1: // 点亮中间2个LED
            if (LED_COUNT >= 4) {
                ws2812_Set(LED_COUNT-1, r, g, b);
                ws2812_Set(LED_COUNT-2, r, g, b);
            }
            break;

        case 2: // 点亮最右边2个LED
            if (LED_COUNT >= 2) {
                ws2812_Set(LED_COUNT-3, r, g, b);
                ws2812_Set(LED_COUNT-4, r, g, b);
            }
            break;

        case 3: // 全部熄灭
            // 已经在上面的SetALL中关闭了
            break;
    }

    turn_animation_step++;
    if (turn_animation_step >= 4) {
        turn_animation_step = 0;
    }
}

// 双闪
void ws2812_UpdateHazardSignal(uint8_t r, uint8_t g, uint8_t b)
{
    if (turn_animation_phase == 0) {
        // 点亮所有LED
        ws2812_SetALL(255, 255, 0);
        turn_animation_phase = 1;
    } else {
        // 关闭所有LED
        ws2812_SetALL(0, 0, 0);
        turn_animation_phase = 0;
    }
}















