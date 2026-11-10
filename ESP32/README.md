# ESP32（小智 AI）集成文件说明

本目录不是完整的小智工程，而是**对上游 xiaozhi-esp32-main 的增量修改**。

## 使用方法

1. 获取上游工程：

   ```bash
   git clone --recursive https://github.com/78/xiaozhi-esp32.git xiaozhi-esp32-main
   ```

2. 将本目录内容覆盖到上游工程对应位置：

   | 本仓库文件 | 目标位置（上游工程内） | 说明 |
   |-----------|----------------------|------|
   | `boards/bread-compact-wifi-lcd/compact_wifi_board_lcd.cc` | `main/boards/bread-compact-wifi-lcd/` | 自定义板级实现（覆盖上游同名文件） |
   | `boards/bread-compact-wifi-lcd/config.h` | `main/boards/bread-compact-wifi-lcd/` | 引脚配置（覆盖） |
   | `boards/bread-compact-wifi-lcd/config.json` | `main/boards/bread-compact-wifi-lcd/` | 板级构建配置（覆盖） |
   | `boards/bread-compact-wifi-lcd/stm32_uart_device.cc` | `main/boards/bread-compact-wifi-lcd/` | **新增**：UART 通信设备 |
   | `boards/bread-compact-wifi-lcd/stm32_uart_device.h` | `main/boards/bread-compact-wifi-lcd/` | **新增**：UART 设备头文件 |
   | `boards/bread-compact-wifi-lcd/stm32_mcp_device.h` | `main/boards/bread-compact-wifi-lcd/` | **新增**：MCP 协议设备封装 |
   | `boards/bread-compact-wifi-lcd/qf_ws2812b.h` | `main/boards/bread-compact-wifi-lcd/` | **新增**：WS2812B 灯效 |
   | `main/CMakeLists.txt` | `main/` | 构建脚本（已注册 `stm32_uart_device.cc`，可对照修改） |
   | `sdkconfig` | 工程根目录 | ESP32-S3 配置参考（或自行 menuconfig） |

3. 编译烧录：

   ```bash
   idf.py set-target esp32s3
   idf.py menuconfig     # Board Type 选择 bread-compact-wifi-lcd
   idf.py build flash monitor
   ```

## 核心改动

- 板级 `compact_wifi_board_lcd.cc` 中实例化 `Stm32UartDevice`，通过 UART2
  与 STM32 以 115200 波特率通信；
- `Stm32McpDevice` 将 STM32 外设（灯、排风、蜂鸣器、温湿度）注册为 MCP 工具，
  小智 AI 可通过语音指令直接调用；
- `qf_ws2812b.h` 提供 WS2812B 冷暖光调节接口。

## 注意

- `sdkconfig` 为本机生成的参考配置（启用 AFE 唤醒词 + 语音模型），
  重新 menuconfig 后会覆盖；
- 上游工程更新后，`main/CMakeLists.txt` 需重新对照合并。
