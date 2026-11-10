/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include <string.h>
#include "uartmcp.h"
#include "ws2812b.h"
#include "tim.h"
#include "uln2003an.h"
#include "delay.h"
#include "dht11.h"
#include "key.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
data_t g_data;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
	
	//HAL_UART_Transmit(&huart1,"hello ai\r\n",strlen("hello ai\r\n"),1000);
	
		TIM4_Delay_Init();
	
	
	Win_Motor_GPIO_Init();
	HAL_GPIO_WritePin(RELAY1_GPIO_Port, RELAY1_Pin, GPIO_PIN_SET);
	osDelay(400);
	HAL_GPIO_WritePin(RELAY1_GPIO_Port, RELAY1_Pin, GPIO_PIN_RESET);
	osDelay(400);
	HAL_GPIO_WritePin(RELAY2_GPIO_Port, RELAY2_Pin, GPIO_PIN_SET);
	osDelay(400);
	HAL_GPIO_WritePin(RELAY2_GPIO_Port, RELAY2_Pin, GPIO_PIN_RESET);
	osDelay(400);
	HAL_GPIO_WritePin(RELAY3_GPIO_Port, RELAY3_Pin, GPIO_PIN_SET);
	osDelay(400);
	HAL_GPIO_WritePin(RELAY3_GPIO_Port, RELAY3_Pin, GPIO_PIN_RESET);
	osDelay(400);
	win_open(1000);
	osDelay(400);
	win_close(1000);
	osDelay(400);
	

	
	LEDs_Driver_Init();
	WS2812B_Init();
	DHT11_Init();
	mcp_uart_init();

	Key_Init();        /* PB0/1/4/5 keys + PA11 buzzer */
	Key_Task_Start();  /* 20ms key scan task */
	
	LEDs_SetColdBrightness(500);
	LEDs_SetWarmBrightness(500);
	osDelay(400);
	LEDs_SetColdBrightness(0);
	LEDs_SetWarmBrightness(0);
	osDelay(400);
	
	
  for(;;)
  {
		if(g_data.leds_state == 1)
		{

			LEDs_SetColdBrightness(g_data.leds_cold);
			LEDs_SetWarmBrightness(g_data.leds_worm);
		}
		else
		{
			LEDs_SetColdBrightness(0);
			LEDs_SetWarmBrightness(0);
		}
		
		
			if(g_data.gongwei_led_state)
		{
			HAL_GPIO_WritePin(RELAY1_GPIO_Port, RELAY1_Pin, GPIO_PIN_SET);
		}else
		{
			HAL_GPIO_WritePin(RELAY1_GPIO_Port, RELAY1_Pin, GPIO_PIN_RESET);
		}

		if(g_data.gongwei_fan_state)
		{
			HAL_GPIO_WritePin(RELAY2_GPIO_Port, RELAY2_Pin, GPIO_PIN_SET);
		}else
		{
			HAL_GPIO_WritePin(RELAY2_GPIO_Port, RELAY2_Pin, GPIO_PIN_RESET);
		}
		
		if(g_data.kongtian_state)
		{
			HAL_GPIO_WritePin(RELAY3_GPIO_Port, RELAY3_Pin, GPIO_PIN_SET);
		}else
		{
				HAL_GPIO_WritePin(RELAY3_GPIO_Port, RELAY3_Pin, GPIO_PIN_RESET);
		}
		
		
		if(g_data.win_state)
		{
			win_open(1000);      // 内部有状态锁存，已打开时不会重复转动
		}
		else
		{
			win_close(1000);
		}


		DHT11_ReadData(&g_data.humi,&g_data.temp);
		
		
		
    osDelay(500);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

