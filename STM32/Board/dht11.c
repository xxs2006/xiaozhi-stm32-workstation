#include "stm32f1xx_hal.h"
#include "delay.h"          // 假设用户仍提供 Delay_us / Delay_ms，下面会给出基于 HAL 的实现
#include "dht11.h"

// DHT11 GPIO 定义
#define DHT11_GPIO_PORT     GPIOA
#define DHT11_GPIO_PIN      GPIO_PIN_5

// HAL 库中时钟使能由 __HAL_RCC_GPIOA_CLK_ENABLE() 完成，不再需要 RCC_APB2Periph 宏

// 设置引脚为输出模式
static void DHT11_SetOutput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

// 设置引脚为输入模式（上拉输入）
static void DHT11_SetInput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;    // 上拉输入
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

// 设置引脚电平
static void DHT11_SetPin(uint8_t level) {
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, (level ? GPIO_PIN_SET : GPIO_PIN_RESET));
}

// 读取引脚电平
static uint8_t DHT11_ReadPin(void) {
    return (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET) ? 1 : 0;
}

/**
 * 等待DHT11响应
 * @return 成功返回1，超时返回0
 */
static uint8_t DHT11_WaitForResponse(void) {
    uint32_t timeout;
    
    // 等待DHT11拉低
    timeout = 10000;
    while (DHT11_ReadPin() && timeout--);
    if (!timeout) return 0;
    
    // 等待DHT11拉高
    timeout = 10000;
    while (!DHT11_ReadPin() && timeout--);
    if (!timeout) return 0;
    
    // 等待DHT11再次拉低，表示数据传输开始
    timeout = 10000;
    while (DHT11_ReadPin() && timeout--);
    if (!timeout) return 0;
    
    return 1;
}

/**
 * 读取一位数据
 * @return 读取的数据位(0或1)
 */
static uint8_t DHT11_ReadBit(void) {
    uint32_t timeout;
    
    // 等待低电平结束
    timeout = 10000;
    while (!DHT11_ReadPin() && timeout--);
    if (!timeout) return 0;
    
    // 延时40us，判断是0还是1
    Delay_us(40);
    
    // 读取电平
    uint8_t bit = DHT11_ReadPin();
    
    // 等待高电平结束
    timeout = 10000;
    while (DHT11_ReadPin() && timeout--);
    
    return bit;
}

/**
 * 读取一个字节数据
 * @return 读取的字节数据
 */
static uint8_t DHT11_ReadByte(void) {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte <<= 1;
        byte |= DHT11_ReadBit();
    }
    return byte;
}

int DHT11_Init(void) {
    // 使能GPIO时钟（HAL方式）
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // 设置为输出模式
    DHT11_SetOutput();
    
    // 初始状态为高电平
    DHT11_SetPin(1);
    
    return DHT11_OK;
}

int DHT11_ReadData(float *humidity, float *temperature) {
    uint8_t data[5] = {0};
    uint8_t checksum;
    
    // 发送起始信号
    DHT11_SetOutput();
    DHT11_SetPin(0);
    Delay_ms(20);  // 拉低至少18ms
    
    // 释放总线
    DHT11_SetPin(1);
    Delay_us(40);  // 主机拉高20-40us
    
    // 切换为输入模式，等待响应
    DHT11_SetInput();
    if (!DHT11_WaitForResponse()) {
        return DHT11_ERROR_TIMEOUT;
    }
    
    // 读取40位数据
    for (int i = 0; i < 5; i++) {
        data[i] = DHT11_ReadByte();
    }
    
    // 计算校验和
    checksum = data[0] + data[1] + data[2] + data[3];
    
    // 验证校验和
    if (checksum != data[4]) {
        return DHT11_ERROR_CHECKSUM;
    }
    
    // 转换数据
    *humidity = (float)data[0] + (float)data[1] / 10.0f;
    *temperature = (float)data[2] + (float)data[3] / 10.0f;
    
    return DHT11_OK;
}
