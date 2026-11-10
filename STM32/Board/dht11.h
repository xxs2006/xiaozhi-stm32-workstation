#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f1xx_hal.h"   // 改为 HAL 库头文件（但为了兼容，可保留原 stm32f10x.h 的包含，但推荐使用 HAL）

/**
 * DHT11返回值定义
 */
typedef enum {
    DHT11_OK = 0,         // 成功
    DHT11_ERROR_TIMEOUT,  // 响应超时
    DHT11_ERROR_CHECKSUM, // 校验和错误
    DHT11_ERROR_GPIO,     // GPIO配置错误
} DHT11_Status;

/**
 * 初始化DHT11传感器
 * @return 成功返回DHT11_OK，失败返回错误码
 */
int DHT11_Init(void);

/**
 * 读取DHT11温湿度数据
 * @param humidity 指向存储湿度值的float变量
 * @param temperature 指向存储温度值的float变量
 * @return 成功返回DHT11_OK，失败返回错误码
 */
int DHT11_ReadData(float *humidity, float *temperature);

#endif /* __DHT11_H */
