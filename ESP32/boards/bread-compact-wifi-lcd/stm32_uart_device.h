#pragma once

#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 STM32 通信串口 (UART1) ，并创建 RTOS 任务
 * @return ESP_OK 成功，其他失败
 */
esp_err_t stm32_uart_device_init(void);

esp_err_t stm32_uart_send(uint8_t* data, size_t len, int timeout_ms);



typedef struct {
	uint8_t keting_led_state;
	uint8_t woshi_fan_state;
    uint8_t leds_state;             //灯带
    uint8_t kongtian_state;
    uint8_t win_state;

    uint16_t leds_cold;
    uint16_t leds_worm;

    uint32_t RGBleds;

    float temp;
    float humi;
	
}data_t;

extern data_t g_data;




#ifdef __cplusplus
}
#endif