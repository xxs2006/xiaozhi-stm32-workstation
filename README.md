# 基于小智AI（ESP32-S3）+ STM32 的智能工位控制系统

面向硬件工程师工位的 AI 语音助手与智能控制系统。上位机 ESP32-S3 运行小智 AI
（语音唤醒、对话、联网问答），通过 UART + MCP 协议与下位机 STM32F103C8T6
通信，实现环境监测与外设联动（焊台模式开灯排风、冷暖光调节、超温报警等）。

## 系统架构

```
┌─────────────────────┐   UART(115200)   ┌──────────────────────┐
│   ESP32-S3 (小智AI)  │ ◄──────────────► │  STM32F103C8T6       │
│  INMP441 麦克风      │   JSON/MCP 协议  │  DHT11 温湿度        │
│  MAX98357 扬声器     │                  │  WS2812B 彩灯        │
│  Wi-Fi 联网问答      │                  │  ULN2003 继电器组    │
│  stm32_uart_device  │                  │  蜂鸣器(30℃回差报警)  │
└─────────────────────┘                  └──────────────────────┘
```

## 目录结构

```
├── 搭建手册.docx          # 整机搭建、烧录、联调步骤
├── PCB/                   # 立创EDA(专业版) PCB 工程 .epro
├── STM32/                 # Keil MDK 完整工程（可直接编译）
│   ├── pro/
│   │   ├── Board/         # 用户驱动：dht11 / ws2812b / uln2003an /
│   │   │                  #   uartmcp / key(含蜂鸣器回差) / delay
│   │   ├── Core/          # CubeMX 生成的 main / freertos / 外设初始化
│   │   ├── Drivers/       # STM32F1 HAL 库
│   │   ├── Middlewares/   # FreeRTOS
│   │   ├── MDK-ARM/       # Keil 工程文件 (pro.uvprojx)
│   │   └── pro.ioc        # CubeMX 配置
└── ESP32/                 # 小智 AI 侧集成文件（覆盖到 xiaozhi-esp32-main）
    ├── boards/bread-compact-wifi-lcd/   # 自定义板级 + stm32_uart_device 等
    ├── main/CMakeLists.txt              # 已注册 stm32_uart_device.cc
    └── sdkconfig                        # ESP32-S3 配置参考
```

## 主要功能

- **语音交互**：唤醒词"你好小智"，支持连续对话、联网问答
- **MCP 设备联动**：AI 通过 MCP 协议直接控制 STM32 外设（灯光、排风、报警等）
- **环境监测**：DHT11 温湿度采集
- **超温报警**：30℃ 触发蜂鸣器，带 2℃ 回差（防临界抖动），按键手动消音
- **工作模式**：焊接模式（照明+排风）、冷暖光调节等
- **WS2812B 氛围灯**：冷暖色温可调

## 快速开始

### STM32（Keil MDK）

1. 用 Keil5 打开 `STM32/pro/MDK-ARM/pro.uvprojx`
2. 编译并烧录到 STM32F103C8T6
3. 外设接线见 `搭建手册.docx`

### ESP32-S3（小智 AI）

1. 克隆上游项目：`git clone https://github.com/78/xiaozhi-esp32.git xiaozhi-esp32-main`
2. 将 `ESP32/` 下的文件覆盖到对应目录（详见 `ESP32/README.md`）
3. `idf.py set-target esp32s3 && idf.py menuconfig` 选择 ` bread-compact-wifi-lcd` 板级
4. `idf.py build flash monitor`

### 联调

两板上电后通过 UART 交互，详见 `搭建手册.docx`。

## 硬件清单

| 模块 | 型号 |
|------|------|
| 主控(上位机) | ESP32-S3 |
| 主控(下位机) | STM32F103C8T6 |
| 麦克风 | INMP441 (I2S) |
| 功放 | MAX98357 |
| 温湿度 | DHT11 |
| 灯效 | WS2812B |
| 继电器 | ULN2003 驱动 |
| 显示 | 可选 LCD |

## 许可

本项目的小智 AI 部分基于 [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)
（MIT License）修改，修改部分与本仓库其余内容仅用于学习交流。
