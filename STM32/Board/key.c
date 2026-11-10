#include "key.h"
#include "cmsis_os.h"
#include "tim.h"

/*
 * Button wiring (see schematic 2026-10-29):
 *   SW1 -> PB0, SW2 -> PB1, SW3 -> PB4, SW4 -> PB5
 * Each pin has an external 10k pull-up to 3V3 and a 100nF cap.
 * Pressed  = pin low (GPIO_PIN_RESET).
 * Buzzer: PA11, active high.
 */

#define KEY_NUM          4

static const GPIO_TypeDef *key_port[KEY_NUM] = {
    GPIOB, GPIOB, GPIOB, GPIOB
};
static const uint16_t key_pin[KEY_NUM] = {
    GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_4, GPIO_PIN_5
};

#define BUZZER_PORT      GPIOA
#define BUZZER_PIN       GPIO_PIN_11

#define TEMP_ALARM_TH    30.0f   /* auto alarm trigger threshold, Celsius */
#define TEMP_REARM_TH    28.0f   /* re-arm only below this (hysteresis band,
                                  * prevents re-alarm chatter when temp
                                  * fluctuates around the trigger point) */

/* debounce state: two consecutive equal samples (2 x 20 ms) confirm */
typedef struct {
    uint8_t stable;    /* confirmed level: 1 = pressed */
    uint8_t last_raw;  /* previous raw sample */
    uint8_t cnt;       /* consecutive-different counter */
} key_state_t;

static key_state_t ks[KEY_NUM];

/* ---------------- buzzer state ---------------- */
static uint8_t alarm_on   = 0; /* buzzer currently sounding */
static uint8_t auto_armed = 1; /* auto (over-temperature) alarm armed */

static void Buzzer_Apply(void)
{
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN,
                      alarm_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* ---------------- key 1: work light ---------------- */
static void Key1_Toggle(void)
{
    if (g_data.leds_state || g_data.gongwei_led_state) {
        /* off: strip + relay both off */
        g_data.leds_state       = 0;
        g_data.gongwei_led_state = 0;
        LEDs_SetColdBrightness(0);
        LEDs_SetWarmBrightness(0);
    } else {
        /* on: cold 200 / warm 500, relay closed */
        g_data.leds_cold         = 200;
        g_data.leds_worm         = 500;
        g_data.leds_state        = 1;
        g_data.gongwei_led_state = 1;
        LEDs_SetColdBrightness(200);
        LEDs_SetWarmBrightness(500);
    }
    /* RELAY1 is re-applied by defaultTask from g_data.gongwei_led_state */
}

/* ---------------- key 2: fan + window ---------------- */
static void Key2_Toggle(void)
{
    if (g_data.gongwei_fan_state || g_data.win_state) {
        g_data.gongwei_fan_state = 0;
        g_data.win_state         = 0;
    } else {
        g_data.gongwei_fan_state = 1;
        g_data.win_state         = 1;
    }
    /* RELAY2 applied immediately for fast response; defaultTask will
     * re-apply the same level from g_data later. The window motor runs
     * in defaultTask (win_open/win_close block for ~2 s). */
    HAL_GPIO_WritePin(RELAY2_GPIO_Port, RELAY2_Pin,
                      g_data.gongwei_fan_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* ---------------- key 3: RGB strip modes ---------------- */
/* g_data.RGBleds format (consumed by ws2812b_task):
 *   bit31 = enable, bits24..30 = mode, bits23..0 = RGB color
 * mode 0 = static, 1 = single-color breath, 2 = single-dot flow,
 * mode 3 = rainbow breath, mode 4 = rainbow flow (added). */
static uint8_t rgb_step = 0; /* 0=off, 1=rainbow breath, 2=rainbow flow, 3=white solid */

static void Key3_Cycle(void)
{
    rgb_step = (rgb_step + 1) & 0x03;
    switch (rgb_step) {
    case 1: /* rainbow breathing */
        g_data.RGBleds = (1UL << 31) | (3UL << 24);
        break;
    case 2: /* rainbow flowing */
        g_data.RGBleds = (1UL << 31) | (4UL << 24);
        break;
    case 3: /* white solid */
        g_data.RGBleds = (1UL << 31) | (0UL << 24)
                       | (0xFFUL << 16) | (0xFFUL << 8) | 0xFFUL;
        break;
    default: /* off */
        rgb_step = 0;
        g_data.RGBleds = 0;
        break;
    }
}

/* ---------------- key 4: buzzer ---------------- */
static void Key4_Toggle(void)
{
    if (alarm_on) {
        /* silence. If this was an over-temperature alarm, disarm auto
         * alarm until temperature drops back below the threshold. */
        alarm_on = 0;
        if (g_data.temp > TEMP_ALARM_TH)
            auto_armed = 0;
    } else {
        /* manual alarm on (stays on until next press) */
        alarm_on   = 1;
        auto_armed = 1;
    }
    Buzzer_Apply();
}

/* ---------------- dispatcher ---------------- */
static void Key_Event(uint8_t idx)
{
    switch (idx) {
    case 0: Key1_Toggle(); break;
    case 1: Key2_Toggle(); break;
    case 2: Key3_Cycle();  break;
    case 3: Key4_Toggle(); break;
    default: break;
    }
}

/* ---------------- task ---------------- */
static void Key_Task(void *argument)
{
    (void)argument;

    for (;;) {
        /* periodic auto-alarm check (temperature from defaultTask)
         * hysteresis: trigger at >30C, re-arm only below 28C.
         * The 28~30C band is a dead zone, so DHT11 fluctuation
         * around 30C can no longer re-trigger the alarm. */
        if (g_data.temp <= TEMP_REARM_TH)
            auto_armed = 1;             /* well below threshold -> re-arm */
        if (!alarm_on && auto_armed && g_data.temp > TEMP_ALARM_TH) {
            alarm_on = 1;
            Buzzer_Apply();
        }

        /* scan 4 keys with debounce */
        for (uint8_t i = 0; i < KEY_NUM; i++) {
            uint8_t raw = (HAL_GPIO_ReadPin((GPIO_TypeDef *)key_port[i],
                                            key_pin[i]) == GPIO_PIN_RESET) ? 1 : 0;
            if (raw == ks[i].last_raw) {
                if (raw != ks[i].stable) {
                    if (++ks[i].cnt >= 2) {
                        ks[i].stable = raw;
                        ks[i].cnt    = 0;
                        if (raw)      /* press-down edge */
                            Key_Event(i);
                    }
                } else {
                    ks[i].cnt = 0;
                }
            } else {
                ks[i].last_raw = raw;
                ks[i].cnt      = 0;
            }
        }

        osDelay(20); /* 20 ms scan tick */
    }
}

/* ---------------- init ---------------- */
void Key_Init(void)
{
    GPIO_InitTypeDef g = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* keys: floating input, external 10k pull-ups on board */
    g.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &g);

    /* buzzer: push-pull output, keep silent at boot */
    g.Pin   = BUZZER_PIN;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(BUZZER_PORT, &g);
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
}

void Key_Task_Start(void)
{
    const osThreadAttr_t attr = {
        .name       = "keyTask",
        .stack_size = 256 * 4,
        .priority   = osPriorityNormal
    };
    osThreadNew(Key_Task, NULL, &attr);
}
