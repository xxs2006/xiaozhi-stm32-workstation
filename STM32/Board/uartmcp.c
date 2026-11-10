#include "uartmcp.h"

#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern UART_HandleTypeDef huart1;
#define MCP_USART_HANDLE &huart1

#define MCP_UART_RX_BUF_SIZE 256   //
#define MCP_UART_PROC_BUF_SIZE 256 //

uint8_t mcp_uart_rx_dma_buf[MCP_UART_RX_BUF_SIZE] = {0};
uint8_t mcp_uart_proc_buf[MCP_UART_PROC_BUF_SIZE] = {0};
uint16_t mcp_uart_rx_len = 0;
uint8_t mcp_uart_rx_complete_flag = 0;

static osThreadId_t mcp_taskHandle = NULL;
const osThreadAttr_t mcp_taskAttributes = {
    .name = "mcp_task",
    .stack_size = 256 * 2,
    .priority = (osPriority_t)osPriorityNormal,
};

int MCP_send_data(uint8_t *data, uint16_t len)
{
  if (HAL_UART_Transmit(MCP_USART_HANDLE, data, len, 1000) != HAL_OK)
  {
    return -1;
  }
  return len;
}

void MCP_UART_IRQHandler(void)
{

  // 1. �ж��Ƿ񴥷��˴��� �����жϣ�IDLE ��־λ��λ��˵�� MCP һ֡���ݽ�����ɣ�
  if (__HAL_UART_GET_FLAG(MCP_USART_HANDLE, UART_FLAG_IDLE) != RESET)
  {
    // 2. ��������жϱ�־λ��F103 ���밴���� SR �� �� DR��˳�������HAL ���װ�˸��߼���
    __HAL_UART_CLEAR_IDLEFLAG(MCP_USART_HANDLE);

    // 3. ֹͣ DMA ���գ���ֹ MCP �������ݸ��ǻ���������֤���ݰ������ԣ�
    HAL_DMA_Abort(&hdma_usart1_rx);

    // 4. ����ʵ�ʽ��յ� MCP ���ݳ��ȣ���ʽ���ܻ�������С - DMA ʣ��δ�����ֽ�����
    mcp_uart_rx_len = MCP_UART_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);

    // ֪ͨ����
    if (mcp_uart_rx_complete_flag == 0)
    {
      osThreadFlagsSet(mcp_taskHandle, 0x01);
      mcp_uart_rx_complete_flag = 1;
    }

    HAL_UART_Receive_DMA(MCP_USART_HANDLE, mcp_uart_rx_dma_buf, MCP_UART_RX_BUF_SIZE);
  }
}


#define FRAME_ID 0x0001
static uint8_t cmd_rsk[40];
int cmd1001_ask(void)
{
    cmd_rsk[0] = 0xFE;
    cmd_rsk[1] = 0xEF;

    // ID �� �滻Ϊ�궨�� LORA_FRAME_ID
    cmd_rsk[2] = (uint8_t)(FRAME_ID & 0xFF);        // ���ֽ� 0x02
    cmd_rsk[3] = (uint8_t)((FRAME_ID >> 8) & 0xFF); // ���ֽ� 0x00

    // ������ 0x1001
    cmd_rsk[4] = 0x01;
    cmd_rsk[5] = 0x10;

    // data len
    cmd_rsk[6] = 8;

    memcpy(&cmd_rsk[7], &g_data.temp, 4);
    memcpy(&cmd_rsk[11], &g_data.humi, 4);
   

    // �̶�֡β
    cmd_rsk[15] = 0x16;
    cmd_rsk[16] = 0x17;

    return MCP_send_data(cmd_rsk, 17);
}




int mcp_frame_par(uint8_t *data, uint16_t len)
{

  if (len < 9)
  {
    return 0;
  }

  if (data[0] != 0xFE || data[1] != 0xEF)
  {
    return 0;
  }

  uint16_t id;
  uint16_t cmd;
  uint8_t data_len;

  memcpy(&id, &data[2], 2);
  memcpy(&cmd, &data[4], 2);
  memcpy(&data_len, &data[6], 1);

  if (id != 0x0001)
  {
    return 0;
  }

  if (cmd == 0x1001)
  {
    /* Change-detection merge:
     * The ESP32 re-sends its full state every 3 s. Overwriting blindly
     * would wipe out local button toggles. Apply a field only when its
     * value actually CHANGED since the last frame (i.e. a real voice
     * command), so local button actions survive the periodic refresh. */
    static uint8_t  last_bits = 0xFF;      /* force apply on first frame */
    static uint16_t last_cold = 0xFFFF;
    static uint16_t last_worm = 0xFFFF;
    static uint32_t last_rgb  = 0xFFFFFFFF;

    uint8_t bits = data[7];
    if (bits != last_bits)
    {
      last_bits = bits;
      g_data.gongwei_led_state = (bits >> 0) & 0x01;
      g_data.gongwei_fan_state = (bits >> 1) & 0x01;
      g_data.leds_state        = (bits >> 2) & 0x01;
      g_data.kongtian_state    = (bits >> 3) & 0x01;
      g_data.win_state         = (bits >> 4) & 0x01;
    }

    uint16_t cold, worm;
    uint32_t rgb;
    memcpy(&cold, &data[8], 2);
    memcpy(&worm, &data[10], 2);
    memcpy(&rgb,  &data[12], 4);
    if (cold != last_cold) { last_cold = cold; g_data.leds_cold = cold; }
    if (worm != last_worm) { last_worm = worm; g_data.leds_worm = worm; }
    if (rgb  != last_rgb)  { last_rgb  = rgb;  g_data.RGBleds  = rgb;  }

    cmd1001_ask();
  }

  return 1;
}

/* ���� ���� ִ�� Ӧ�� */
void mcp_task(void *arg)
{

  uint32_t flags;
  while (1)
  {
    flags = osThreadFlagsWait(0xff, osFlagsWaitAny, osWaitForever);
    if (flags == 0x01)
    {

      mcp_frame_par(mcp_uart_rx_dma_buf, mcp_uart_rx_len);
      mcp_uart_rx_complete_flag = 0;
    }
  }
}

void mcp_uart_init(void)
{

  HAL_UART_Receive_DMA(MCP_USART_HANDLE, mcp_uart_rx_dma_buf, MCP_UART_RX_BUF_SIZE);
  __HAL_UART_ENABLE_IT(MCP_USART_HANDLE, UART_IT_IDLE);

  mcp_taskHandle = osThreadNew(mcp_task, NULL, &mcp_taskAttributes);
}
