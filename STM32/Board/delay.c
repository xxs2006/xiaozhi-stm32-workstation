#include "delay.h"
#include "stm32f1xx_hal.h"

// 全局TIM4句柄（HAL库操作定时器依赖该句柄，需全局定义方便延时函数访问）
TIM_HandleTypeDef htim4;

/**
  * @brief  初始化TIM4用于延时功能（HAL库版）
  * @param  无
  * @retval 无
  */
void TIM4_Delay_Init(void)
{
  // 定时器时基配置结构体（对应标准库的TIM_TimeBaseInitTypeDef）
  TIM_Base_InitTypeDef TIM_TimeBaseStructure = {0};

  // 1. 使能TIM4时钟（HAL库宏定义，对应标准库的RCC_APB1PeriphClockCmd）
  __HAL_RCC_TIM4_CLK_ENABLE();

  // 2. 配置定时器时基参数，和原标准库逻辑一致
  // 定时器时钟频率 = 72MHz（STM32F1系统时钟72MHz，APB1预分频为2时，TIM4时钟=36MHz×2=72MHz）
  // 分频系数 = 72 - 1，得到1MHz的计数频率（1us计数一次）
  TIM_TimeBaseStructure.Prescaler = 72 - 1;          // 预分频器
  TIM_TimeBaseStructure.CounterMode = TIM_COUNTERMODE_UP; // 向上计数模式
  TIM_TimeBaseStructure.Period = 0xFFFF;             // 自动重装值（最大65535，对应原标准库）
  TIM_TimeBaseStructure.ClockDivision = TIM_CLOCKDIVISION_DIV1; // 时钟分频（无分频，对应原标准库的0）
  TIM_TimeBaseStructure.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE; // 关闭自动重装预装载

  // 3. 初始化TIM4（HAL库定时器初始化函数）
  htim4.Instance = TIM4;
  htim4.Init = TIM_TimeBaseStructure;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    // 初始化失败可添加错误处理（比如死循环、LED报警等）
    while(1);
  }

  // 4. 启动TIM4计数器（HAL库启动定时器函数，对应标准库的TIM_Cmd）
  HAL_TIM_Base_Start(&htim4);
}

/**
  * @brief  微秒级延时函数（HAL库版，基于TIM4）
  * @param  us: 要延时的微秒数，范围0-65535
  * @retval 无
  */
void Delay_us(uint16_t us)
{
  uint16_t cnt_start, cnt_now, cnt_diff;

  // 记录当前TIM4计数器值（HAL库直接访问寄存器，和标准库用法一致，保证延时精度）
  cnt_start = (uint16_t)__HAL_TIM_GET_COUNTER(&htim4);

  while (1)
  {
    // 读取当前计数器值
    cnt_now = (uint16_t)__HAL_TIM_GET_COUNTER(&htim4);

    // 处理计数器溢出情况（逻辑和原标准库完全一致，无改动）
    if (cnt_now >= cnt_start)
      cnt_diff = cnt_now - cnt_start;
    else
      cnt_diff = 0xFFFF - cnt_start + cnt_now + 1;

    // 达到指定延时时间则退出循环
    if (cnt_diff >= us)
      break;
  }
}

/**
  * @brief  毫秒级延时函数（HAL库版，基于TIM4）
  * @param  ms: 要延时的毫秒数
  * @retval 无
  */
void Delay_ms(uint32_t ms)
{
  while (ms--)
  {
    // 调用1000微秒（1毫秒）延时，逻辑和原标准库一致
    Delay_us(1000);
  }
}
