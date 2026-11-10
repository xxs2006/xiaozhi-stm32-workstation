#include "delay.h"            // 
#include "uln2003an.h"
#include "stm32f1xx_hal.h"    //


#define IN1_PIN    GPIO_PIN_8
#define IN2_PIN    GPIO_PIN_9
#define IN3_PIN    GPIO_PIN_14
#define IN4_PIN    GPIO_PIN_15
#define MOTOR_PORT GPIOB


const uint8_t STEP_SEQUENCE[8] = {
    0x09, // IN1=1, IN2=0, IN3=0, IN4=1 (0b1001)
    0x01, // IN1=1, IN2=0, IN3=0, IN4=0 (0b0001)
    0x03, // IN1=1, IN2=1, IN3=0, IN4=0 (0b0011)
    0x02, // IN1=0, IN2=1, IN3=0, IN4=0 (0b0010)
    0x06, // IN1=0, IN2=1, IN3=1, IN4=0 (0b0110)
    0x04, // IN1=0, IN2=0, IN3=1, IN4=0 (0b0100)
    0x0C, // IN1=0, IN2=0, IN3=1, IN4=1 (0b1100)
    0x08  // IN1=0, IN2=0, IN3=0, IN4=1 (0b1000)
};


void Win_Motor_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 使能 GPIOB 时钟
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // 配置 PB8, PB9, PB14, PB15 为推挽输出
    GPIO_InitStruct.Pin = IN1_PIN | IN2_PIN | IN3_PIN | IN4_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;   // 高速
    HAL_GPIO_Init(MOTOR_PORT, &GPIO_InitStruct);

    // 初始全部拉低
    HAL_GPIO_WritePin(MOTOR_PORT, IN1_PIN | IN2_PIN | IN3_PIN | IN4_PIN, GPIO_PIN_RESET);
}

// 设置步进电机相位
void SetMotorPhase(uint8_t phase) {
    HAL_GPIO_WritePin(MOTOR_PORT, IN1_PIN, (phase & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_PORT, IN2_PIN, (phase & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_PORT, IN3_PIN, (phase & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_PORT, IN4_PIN, (phase & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// 步进电机正转
void Motor_Forward(uint32_t steps, uint32_t delay_us) {
    static int8_t current_step = 0;   // 保留静态变量，使多次调用连续

    for (uint32_t i = 0; i < steps; i++) {
        current_step = (current_step + 1) % 8;
        SetMotorPhase(STEP_SEQUENCE[current_step]);
        Delay_us(delay_us);           // 微秒延时（需外部提供）
    }
}

// 步进电机反转
void Motor_Reverse(uint32_t steps, uint32_t delay_us) {
    static int8_t current_step = 0;

    for (uint32_t i = 0; i < steps; i++) {
        current_step = (current_step == 0) ? 7 : (current_step - 1);
        SetMotorPhase(STEP_SEQUENCE[current_step]);
        Delay_us(delay_us);
    }
}

// 停止电机
void Motor_Stop(void) {
    HAL_GPIO_WritePin(MOTOR_PORT, IN1_PIN | IN2_PIN | IN3_PIN | IN4_PIN, GPIO_PIN_RESET);
}

// ---------- 窗户控制
static uint8_t win_state = 0;   // 0=关闭，1=打开

void win_open(uint32_t speed) {
    if (win_state == 0) {
        Motor_Forward(2048 * 1, speed);   // 1圈
        Delay_ms(1000);                   // 停顿1秒
        Motor_Stop();
        win_state = 1;
    }
}

void win_close(uint32_t speed) {
    if (win_state == 1) {
        Motor_Reverse(2048 * 1, speed);
        Delay_ms(1000);
        Motor_Stop();
        win_state = 0;
    }
}
