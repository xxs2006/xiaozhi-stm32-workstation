#ifndef __WS2812B_H
#define __WS2812B_H

#include "main.h"

/* WS2812B 灯带驱动：通过 TIM2_CH2 的 PWM + DMA 输出逐像素数据。 */
/* 灯珠数量及每颗灯珠的数据位数（R、G、B 各 8 位）。 */
#define WS2812B_LED_COUNT       8
#define WS2812B_BITS_PER_LED   24


/* 软件颜色缓存；发送时驱动会按灯带要求重排为 GRB 顺序。 */
typedef struct
{
    uint8_t G;
    uint8_t R;
    uint8_t B;
} WS2812B_Color;

void WS2812B_Init(void);

/* 修改指定灯珠的颜色；超出灯带范围的 index 会被忽略。 */
void WS2812B_SetPixel(uint16_t index,
                      uint8_t r,
                      uint8_t g,
                      uint8_t b);

void WS2812B_SetAll(uint8_t r,
                    uint8_t g,
                    uint8_t b);

void WS2812B_Clear(void);

/* 编码当前颜色缓存，并启动 TIM2_CH2 DMA 非阻塞发送。 */
void WS2812B_Show(void);

void WS2812B_SetColor(uint8_t r,
                      uint8_t g,
                      uint8_t b);

#endif
