#include "ws2812b.h"
#include "tim.h"
#include <string.h>
#include "cmsis_os2.h"

// ʹ�� TIM2_CH2 ��� PWM ���壬ÿ�� WS2812B ����λ��Ӧһ���Ƚ�ֵ��
// ��ǰ��ʱ��ʱ��Ϊ 72 MHz��ARR Ϊ 89�����ÿ������λ����Ϊ 1.25 us��



/*
====================================================
 WS2812B ����
====================================================
*/

#define WS2812B_0_CODE      28
#define WS2812B_1_CODE      56

/*
====================================================
 LED ����
====================================================
*/

static WS2812B_Color led_buffer[WS2812B_LED_COUNT];

/*
====================================================
 DMA PWM ������
====================================================
*/

// ���� 64 ��ռ�ձ�Ϊ 0 �����ڣ�ʹ�����������һλ�󱣳ֵ͵�ƽԼ 80 us��
#define WS2812B_RESET_BITS    64

#define WS2812B_DATA_LEN  \
	(WS2812B_LED_COUNT * 24 + WS2812B_RESET_BITS)

// ������ÿ��Ԫ�ض�Ӧһ�� PWM ���ڣ�DMA ����Щռ�ձ��������� TIM2_CH2��
static uint16_t ws2812_dma_buf[WS2812B_DATA_LEN];




/*
====================================================
 ���õ��� LED
====================================================
*/

void WS2812B_SetPixel(uint16_t index,
                      uint8_t r,
                      uint8_t g,
                      uint8_t b)
{
    if (index >= WS2812B_LED_COUNT)
        return;

    led_buffer[index].R = r;
    led_buffer[index].G = g;
    led_buffer[index].B = b;
}

/*
====================================================
 ����ȫ�� LED
====================================================
*/

void WS2812B_SetAll(uint8_t r,
                    uint8_t g,
                    uint8_t b)
{
    for (uint16_t i = 0; i < WS2812B_LED_COUNT; i++)
    {
        led_buffer[i].R = r;
        led_buffer[i].G = g;
        led_buffer[i].B = b;
    }
}

/*
====================================================
 ���
====================================================
*/

void WS2812B_Clear(void)
{
    WS2812B_SetAll(0, 0, 0);
}

/*
====================================================
 ���õ�ɫ
====================================================
*/

void WS2812B_SetColor(uint8_t r,
                      uint8_t g,
                      uint8_t b)
{
    WS2812B_SetAll(r, g, b);
}

/*
====================================================
 ���� DMA PWM ����
====================================================

WS2812B ����˳��

G7 G6 G5 G4 G3 G2 G1 G0
R7 R6 R5 R4 R3 R2 R1 R0
B7 B6 B5 B4 B3 B2 B1 B0

ע�⣺

WS2812B �� GRB������ RGB��
====================================================
*/

static void WS2812B_BuildBuffer(void)
{
    uint32_t index = 0;

    // ���ƴ�Ҫ��� GRB ˳����ÿ�ŵ�������ݣ��������λ��ʼ���͡�

    for (uint16_t led = 0;
         led < WS2812B_LED_COUNT;
         led++)
    {
        uint8_t data[3];

        // WS2812B˳��GRB
        data[0] = led_buffer[led].G;
        data[1] = led_buffer[led].R;
        data[2] = led_buffer[led].B;

        for (uint8_t byte = 0;
             byte < 3;
             byte++)
        {
            for (int8_t bit = 7;
                 bit >= 0;
                 bit--)
            {
                if (data[byte] & (1 << bit))
                {
                    ws2812_dma_buf[index++] = WS2812B_1_CODE;
                }
                else
                {
                    ws2812_dma_buf[index++] = WS2812B_0_CODE;
                }
            }
        }
    }

    // �������80us���ҵĵ͵�ƽRESET
    // ������ĩβ׷��ռ�ձ�Ϊ 0 �����ڣ��ṩ��λ/��������ĵ͵�ƽʱ�䡣
    while (index < WS2812B_DATA_LEN)
    {
        ws2812_dma_buf[index++] = 0;
    }
}

/*
====================================================
 ��������
====================================================
*/

void WS2812B_Show(void)
{
    WS2812B_BuildBuffer();

    // �˴����������� DMA ���ͣ�������ɺ��ɻص�����ֹͣ PWM��
    HAL_TIM_PWM_Start_DMA(
        &htim2,
        TIM_CHANNEL_2,
        (uint32_t *)ws2812_dma_buf,
        WS2812B_DATA_LEN
    );
}



/* rainbow color wheel: pos 0..255 -> R/G/B (Adafruit-style wheel) */
static void WS2812B_Wheel(uint8_t pos, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (pos < 85) {
        *r = 255 - pos * 3;
        *g = pos * 3;
        *b = 0;
    } else if (pos < 170) {
        pos -= 85;
        *r = 0;
        *g = 255 - pos * 3;
        *b = pos * 3;
    } else {
        pos -= 170;
        *r = pos * 3;
        *g = 0;
        *b = 255 - pos * 3;
    }
}

void ws2812b_task(void *argument)
{
    // ��������ˮ��״̬��������̬���֣�
    static uint8_t breathBrightness = 0;
    static int8_t breathStep = 1;
    // �����Ч״̬������ģʽ�����ȣ��Լ���ˮģʽ��ǰ�����ĵ�����š�
    static uint16_t flowPos = 0;
	
	
	
	WS2812B_SetAll(0,0,55);
	WS2812B_Show();
	osDelay(2000);
	
	

    for (;;) {
        // RGBleds �� uartmcp.c �еĴ���Э�����������¡�
        uint32_t ctrl = g_data.RGBleds;     // ��ȡ������

        // ��ȡ�� 8 λ������Ϣ
        // �����ָ�ʽ��bit 31 Ϊʹ��λ��bits 30:24 Ϊģʽ��bits 23:0 Ϊ RGB ��ɫֵ��
        uint8_t enable = (ctrl >> 31) & 0x01;          // bit31���ܿ���
        uint8_t mode   = (ctrl >> 24) & 0x7F;          // bits24~30��ģʽ

        if (!enable) {
            // ���عرգ�ȫ��������ʾ
            WS2812B_Clear();
            WS2812B_Show();
            osDelay(50);
            continue;
        }

        // ��ȡ��ɫ���� 24 λ��
        uint8_t r = (ctrl >> 16) & 0xFF;
        uint8_t g = (ctrl >>  8) & 0xFF;
        uint8_t b = (ctrl >>  0) & 0xFF;

        switch (mode) {
            case 0: { // ��̬ģʽ�����е�����ʾ��ͬ��ɫ��
                WS2812B_SetAll(r, g, b);
                WS2812B_Show();
                break;
            }
            case 1: { // ����ģʽ�������������䣬�γɺ���Ч����
                // �������ȣ�����������
                breathBrightness += breathStep;
                if (breathBrightness >= 255) {
                    breathBrightness = 255;
                    breathStep = -1;
                } else if (breathBrightness <= 0) {
                    breathBrightness = 0;
                    breathStep = 1;
                }
                // ������ɫ
                uint8_t r_breath = (r * breathBrightness) / 255;
                uint8_t g_breath = (g * breathBrightness) / 255;
                uint8_t b_breath = (b * breathBrightness) / 255;
                WS2812B_SetAll(r_breath, g_breath, b_breath);
                WS2812B_Show();
                break;
            }
            case 2: { // ��ˮģʽ������һ�ŵ����ǰ�ƣ���ĩβʱ�ص���㡣
                WS2812B_Clear();
                WS2812B_SetPixel(flowPos, r, g, b);
                flowPos++;
                if (flowPos >= WS2812B_LED_COUNT) {
                    flowPos = 0;
                }
                WS2812B_Show();
                break;
            }
            case 3: { /* rainbow breath: hue slowly cycles + brightness envelope */
                static uint8_t  hue    = 0;
                static uint8_t  breath = 4;
                static int8_t   dir    = 1;
                static uint16_t tick   = 0;
                uint8_t r_w, g_w, b_w;

                breath = (uint8_t)(breath + dir * 4); /* ~3.2 s per breath cycle */
                if (breath >= 252) { breath = 252; dir = -1; }
                else if (breath <= 4) { breath = 4; dir = 1; }

                if (++tick >= 6) {            /* hue +1 every 300 ms */
                    tick = 0;
                    hue++;
                }
                WS2812B_Wheel(hue, &r_w, &g_w, &b_w);
                WS2812B_SetAll((r_w * breath) / 255,
                               (g_w * breath) / 255,
                               (b_w * breath) / 255);
                WS2812B_Show();
                break;
            }
            case 4: { /* rainbow flow: 8 LEDs with shifted hues, drifting */
                static uint8_t  hue  = 0;
                static uint16_t tick = 0;
                uint8_t r_w, g_w, b_w;

                if (++tick >= 4) {            /* drift one step every 200 ms */
                    tick = 0;
                    hue  = hue + 8;
                }
                for (uint16_t i = 0; i < WS2812B_LED_COUNT; i++) {
                    WS2812B_Wheel((uint8_t)(hue + i * 32), &r_w, &g_w, &b_w);
                    WS2812B_SetPixel(i, r_w, g_w, b_w);
                }
                WS2812B_Show();
                break;
            }
            default: { // δ֪ģʽ��ȫ������
                WS2812B_Clear();
                WS2812B_Show();
                break;
            }
        }

        osDelay(50);    // 50ms ˢ������
    }
}



/*
====================================================
 ��ʼ��
====================================================
*/

// �����ɫ���棬������������µƴ��� RTOS ����
void WS2812B_Init(void)
{
    WS2812B_Clear();
	
	
	
	const osThreadAttr_t task_attr = {
            .name = "LEDTask",
            .stack_size = 256,
            .priority = osPriorityNormal
        };
        osThreadNew(ws2812b_task, NULL, &task_attr);
	
}












void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        // DMA �ѷ��������ݺ͸�λ�͵�ƽ��ֹͣ PWM ������ͷ�ͨ����
        HAL_TIM_PWM_Stop_DMA(
            &htim2,
            TIM_CHANNEL_2
        );
    }
}

