#include "stm32_uart_device.h"

#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <cstring>

static const char* TAG = "STM32_UART";

// -------------------- 引脚定义 --------------------
static const gpio_num_t UART_TX_PIN = GPIO_NUM_17;
static const gpio_num_t UART_RX_PIN = GPIO_NUM_18;
#define UART_NUM UART_NUM_1
#define UART_BAUD_RATE 115200
#define UART_BUF_SIZE 1024

static const gpio_num_t GPIO_PIN_1 = GPIO_NUM_10;
static const gpio_num_t GPIO_PIN_2 = GPIO_NUM_11;
static const gpio_num_t GPIO_PIN_3 = GPIO_NUM_12;
static const gpio_num_t GPIO_PIN_4 = GPIO_NUM_13;
#define GPIO_DEFAULT_LEVEL 0

// -------------------- 任务与队列 --------------------
static TaskHandle_t uart_task_handle = NULL;
static QueueHandle_t uart_rx_queue = NULL;
#define RX_QUEUE_LEN 10
#define RX_QUEUE_ITEM_SIZE sizeof(uart_event_t)

// -------------------- 互斥锁（用于发送） --------------------
static SemaphoreHandle_t uart_mutex = NULL;

// -------------------- 任务函数 --------------------
static void uart_task(void* arg) {
    ESP_LOGI(TAG, "UART task started, stack water mark: %d", uxTaskGetStackHighWaterMark(NULL));

    uart_event_t event;
    uint8_t* rx_buffer = (uint8_t*)malloc(UART_BUF_SIZE);
    if (!rx_buffer) {
        ESP_LOGE(TAG, "Failed to allocate rx buffer");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        ESP_LOGI(TAG, "STM32 UART RX RUN!!.");

        if (xQueueReceive(uart_rx_queue, &event, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Event type: %d", event.type);
            switch (event.type) {
                case UART_DATA: {
                    size_t length = 0;
                    uart_get_buffered_data_len(UART_NUM, &length);
                    ESP_LOGI(TAG, "Data length: %d", length);
                    if (length > 0) {
                        int len = uart_read_bytes(UART_NUM, rx_buffer, length, 0);
                        if (len > 0) {
                            ESP_LOGI(TAG, "Received %d bytes: %.*s", len, len, rx_buffer);

                            if (rx_buffer[0] == 0xfe && rx_buffer[1] == 0xef) {
                                // 温度：7~10字节
                                memcpy(&g_data.temp, &rx_buffer[7], 4);
                                // 湿度：11~14字节
                                memcpy(&g_data.humi, &rx_buffer[11], 4);
                            }
                        }
                    }
                    break;
                }
                case UART_FIFO_OVF:
                    ESP_LOGW(TAG, "UART FIFO overflow");
                    uart_flush_input(UART_NUM);
                    xQueueReset(uart_rx_queue);
                    break;
                case UART_BUFFER_FULL:
                    ESP_LOGW(TAG, "UART buffer full");
                    uart_flush_input(UART_NUM);
                    xQueueReset(uart_rx_queue);
                    break;
                default:
                    ESP_LOGI(TAG, "Other event: %d", event.type);
                    break;
            }
        }
    }
    free(rx_buffer);
    vTaskDelete(NULL);
}

// ==================== 公共发送函数 ====================
esp_err_t stm32_uart_send(uint8_t* data, size_t len, int timeout_ms) {
    if (!data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (uart_mutex == NULL) {
        ESP_LOGE(TAG, "UART mutex not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // 获取互斥锁
    TickType_t ticks = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    if (xSemaphoreTake(uart_mutex, ticks) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to acquire UART mutex (timeout)");
        return ESP_ERR_TIMEOUT;
    }

    // 发送数据
    int bytes_written = uart_write_bytes(UART_NUM, (const char*)data, len);
    if (bytes_written < 0) {
        ESP_LOGE(TAG, "uart_write_bytes failed");
        xSemaphoreGive(uart_mutex);
        return ESP_FAIL;
    }

    // 等待发送完成（超时 100ms，可根据波特率调整）
    esp_err_t err = uart_wait_tx_done(UART_NUM, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "uart_wait_tx_done returned %s", esp_err_to_name(err));
    }

    xSemaphoreGive(uart_mutex);
    return ESP_OK;
}

// 基于小智ai的智能家居 的协议设计   参考清风LoRa的协议设计
data_t g_data;
// ===================== 发送函数 =====================
int stm32_cmd1001_send(uint16_t slave_id) {
    // 固定帧格式
    uint8_t send_buf[40] = {0};
    uint8_t data_byte = 0;
    uint16_t send_len = 0;

    // 1. 固定帧头 FE EF
    send_buf[send_len++] = 0xFE;
    send_buf[send_len++] = 0xEF;

    // 2. 从机ID（小端序）
    send_buf[send_len++] = (uint8_t)(slave_id & 0xFF);
    send_buf[send_len++] = (uint8_t)((slave_id >> 8) & 0xFF);

    // 3. 功能码 0x1001 → 01 10
    send_buf[send_len++] = 0x01;
    send_buf[send_len++] = 0x10;

    // 4. 数据长度 01
    send_buf[send_len++] = 9;

    // 5. 拼接状态位
    data_byte = 0;
    data_byte |= (g_data.keting_led_state & 0x01) << 0;  //
    data_byte |= (g_data.woshi_fan_state & 0x01) << 1;   //
    data_byte |= (g_data.leds_state & 0x01) << 2;        //
    data_byte |= (g_data.kongtian_state & 0x01) << 3;    //
    data_byte |= (g_data.win_state & 0x01) << 4;         //

    send_buf[send_len++] = data_byte;

    memcpy(&send_buf[send_len], &g_data.leds_cold, 2);
    send_len = send_len + 2;

    memcpy(&send_buf[send_len], &g_data.leds_worm, 2);
    send_len = send_len + 2;

    memcpy(&send_buf[send_len], &g_data.RGBleds, 4);
    send_len = send_len + 4;

    // 6. 固定CRC 16 16
    send_buf[send_len++] = 0x16;
    send_buf[send_len++] = 0x17;

    // 7. ESP32串口2发送
    stm32_uart_send(send_buf, send_len, 1000);

    // 打印调试
    printf("stm32 - uart2 发送 ");
    for (int i = 0; i < send_len; i++)
        printf(" %02X ", send_buf[i]);

    printf("\r\n");

    return send_len;
}

static void uart_send_task(void* arg) {
    uint16_t slave_id = 0x0001;              // 假设从机ID为1
    TickType_t delay = pdMS_TO_TICKS(3000);  // 每5秒发送一次
    while (1) {
        stm32_cmd1001_send(slave_id);
        vTaskDelay(delay);
    }
}

// ==================== 初始化函数 ====================
esp_err_t stm32_uart_device_init(void) {
    ESP_LOGI(TAG, "Initializing UART1 and GPIOs...");

    // 创建互斥锁
    uart_mutex = xSemaphoreCreateMutex();
    if (uart_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    // 1. 配置 UART1
    uart_config_t uart_cfg = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t err = uart_param_config(UART_NUM, &uart_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_driver_install(UART_NUM, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, RX_QUEUE_LEN,
                              &uart_rx_queue, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    // 2. 初始化 4 个 GPIO（输出）
    gpio_config_t gpio_cfg = {
        .pin_bit_mask = (1ULL << GPIO_PIN_1) | (1ULL << GPIO_PIN_2) | (1ULL << GPIO_PIN_3) |
                        (1ULL << GPIO_PIN_4),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    err = gpio_config(&gpio_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(err));
        return err;
    }
    gpio_set_level(GPIO_PIN_1, GPIO_DEFAULT_LEVEL);
    gpio_set_level(GPIO_PIN_2, GPIO_DEFAULT_LEVEL);
    gpio_set_level(GPIO_PIN_3, GPIO_DEFAULT_LEVEL);
    gpio_set_level(GPIO_PIN_4, GPIO_DEFAULT_LEVEL);

    // 3. 创建 RTOS 任务
    BaseType_t ret =
        xTaskCreatePinnedToCore(uart_task, "uart_task", 4096, NULL, 5, &uart_task_handle,
                                1  // 建议运行在核心 1，避免干扰 WiFi/蓝牙
        );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create uart_send_task");
        // 可选返回错误，但非致命，可以继续
    }

    ret = xTaskCreatePinnedToCore(uart_send_task, "uart_send_task", 4096, NULL,
                                  4,  // 优先级略低于uart_task
                                  NULL, 1);

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "xTaskCreatePinnedToCore failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "STM32 UART and GPIO initialization done.");
    return ESP_OK;
}