#pragma once

#include <esp_log.h>
#include "mcp_server.h"
#include "stm32_uart_device.h"

static const char* HOME_MCP_TAG = "STM32_MCP";

class STM32Mcp {
public:
    /*
        =================================================
        设备支持能力
        一个设备可以有多个能力
    */
    enum DeviceCapability {
        CAP_SWITCH = 0x01,     // 支持“开关控制”
        CAP_SET_PARAM = 0x02,  // 支持“设置参数”
        CAP_GET_PARAM = 0x04   // 支持“读取参数”
    };

    /*
    =================================================
    设备信息结构
    ESP32维护
    =================================================
    */
    typedef struct {
        uint16_t node_id;     // STM32节点ID
        uint16_t device_id;   // STM32内部设备编号
        const char* name;     // AI识别名称
        uint32_t capability;  // 支持能力
    } HomeDevice;

    std::array<HomeDevice, 7> devices = {
        HomeDevice{1, 1, "客厅灯", CAP_SWITCH},
        HomeDevice{1, 2, "排风", CAP_SWITCH},
        HomeDevice{1, 3, "灯带", CAP_SWITCH | CAP_SET_PARAM},
        HomeDevice{1, 4, "温湿度", CAP_GET_PARAM},
        HomeDevice{1, 5, "空调", CAP_SWITCH},
        HomeDevice{1, 6, "全彩灯", CAP_SWITCH | CAP_SET_PARAM},
        HomeDevice{1, 7, "窗户", CAP_SWITCH},
    };

    HomeDevice* FindDevice(const char* name) {
        for (auto& device : devices) {
            if (strcmp(device.name, name) == 0) {
                return &device;
            }
        }
        return nullptr;
    }

    void Register() {
        auto& server = McpServer::GetInstance();
        /**************************************************
         * 1. 设备开关控制
         *
         **************************************************/

        server.AddTool(
            "设备.开关控制",
            "控制设备的开启或关闭。支持的设备:客厅灯、排风、灯带、空调、窗户、全彩灯。"
            "太热了是打开空调 透透气是开窗户然后开排风"
            "客厅灯 客厅里的灯 灯带是冷暖光灯带"
            "状态值:0="
            "关闭，1="
            "开启 ",

            PropertyList({{"设备名称", kPropertyTypeString}, {"状态", kPropertyTypeInteger}}),
            [this](const PropertyList& params) -> ReturnValue {
                std::string name = params["设备名称"].value<std::string>();
                int state = params["状态"].value<int>();
                HomeDevice* device = FindDevice(name.c_str());
                if (device == nullptr) {
                    return "{\"错误\":\"设备不存在\"}";
                }
                if (!(device->capability & CAP_SWITCH)) {
                    return "{\"错误\":\"设备不支持开关\"}";
                }
                ESP_LOGI(HOME_MCP_TAG, "设备:%s node:%d device:%d 状态:%d", device->name,
                         device->node_id, device->device_id, state);

                if (device->device_id == 1) {
                    if (state)
                        g_data.keting_led_state = 1;
                    else
                        g_data.keting_led_state = 0;

                } else if (device->device_id == 2) {
                    if (state)
                        g_data.woshi_fan_state = 1;
                    else
                        g_data.woshi_fan_state = 0;
                } else if (device->device_id == 3) {
                    if (state)
                        g_data.leds_state = 1;
                    else
                        g_data.leds_state = 0;
                } else if (device->device_id == 5) {
                    if (state)
                        g_data.kongtian_state = 1;
                    else
                        g_data.kongtian_state = 0;
                } else if (device->device_id == 6) {
                    if (state)
                        g_data.RGBleds |= (1u << 31);
                    else
                        g_data.RGBleds &= ~(1u << 31);
                } else if (device->device_id == 7) {
                    if (state)
                        g_data.win_state = 1;
                    else
                        g_data.win_state = 0;
                }

                return "{\"状态\":\"设备控制完成\"}";
            });

        /**************************************************
         * 2. 设置设备参数
         *
         **************************************************/
        server.AddTool(
            "设备.设置参数",
            "设置设备的可调参数，参数说明如下：\n"
            "- 灯带亮度：参数名称填'brightness_cold'或'brightness_warm'，数值范围 0-1000\n"
            "晚安-要开氛围暖光灯 设置为10 "
            "- 全彩灯模式：参数名称填'mode'，数值: 常亮=0, 呼吸=1, 流水=2\n",
            PropertyList({{"设备名称", kPropertyTypeString},
                          {"参数名称", kPropertyTypeString},
                          {"数值", kPropertyTypeInteger}}),

            [this](const PropertyList& params) -> ReturnValue {
                std::string name = params["设备名称"].value<std::string>();
                std::string param = params["参数名称"].value<std::string>();
                int value = params["数值"].value<int>();
                HomeDevice* device = FindDevice(name.c_str());
                if (device == nullptr) {
                    return "{\"错误\":\"设备不存在\"}";
                }
                ESP_LOGI(HOME_MCP_TAG, "设备:%s 参数:%s 数值:%d", device->name, param.c_str(),
                         value);

                if (device->device_id == 3) {
                    if (param == "brightness_cold") {
                        g_data.leds_cold = (uint16_t)value;
                    } else if (param == "brightness_warm") {
                        g_data.leds_worm = (uint16_t)value;
                    }
                }

                if (device->device_id == 6) {
                    if (param == "mode") {
                        // 1. 清除高8位中的低7位（bit24~bit30），保留 bit31 和 低24位
                        //    掩码 ~(0x7Fu << 24)  = 0x80FFFFFF (bit31=1, bit24-30=0, bit0-23=1)
                        g_data.RGBleds &= ~(0x7Fu << 24);

                        // 2. 将 mode（范围0~2）左移24位，写入 bit24~bit30
                        //    强制转为 uint32_t 并截断低7位，防止意外污染 bit31
                        g_data.RGBleds |= (((uint32_t)value & 0x7Fu) << 24);
                    }
                }

                return "{\"状态\":\"参数设置完成\"}";
            });

        server.AddTool("设备.设置颜色",
                       "设置全彩灯的颜色，分别指定红、绿、蓝的亮度值，范围均为 0-255。全彩灯 "
                       "彩色灯 彩灯都行 但设备名称是全彩灯",
                       PropertyList({{"设备名称", kPropertyTypeString},
                                     {"红色", kPropertyTypeInteger},
                                     {"绿色", kPropertyTypeInteger},
                                     {"蓝色", kPropertyTypeInteger}}),
                       [this](const PropertyList& params) -> ReturnValue {
                           std::string name = params["设备名称"].value<std::string>();
                           int r = params["红色"].value<int>();
                           int g = params["绿色"].value<int>();
                           int b = params["蓝色"].value<int>();

                           HomeDevice* device = FindDevice(name.c_str());
                           if (device == nullptr) {
                               return "{\"错误\":\"设备不存在\"}";
                           }
                           // 打包成 24 位 RGB，高 8 位（含 mode 和状态位）保留不变
                           uint32_t rgb = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
                           g_data.RGBleds &= 0xFF000000u;          // 清除低24位
                           g_data.RGBleds |= (rgb & 0x00FFFFFFu);  // 写入新颜色

                           ESP_LOGI(HOME_MCP_TAG, "设置颜色 R=%d G=%d B=%d 打包值=0x%06X", r, g, b,
                                    rgb);
                           return "{\"状态\":\"颜色设置完成\"}";
                       });

        /**************************************************
         * . 读取传感器
         *
         * 温度
         * 湿度
         * 光照
         *
         **************************************************/
        server.AddTool(
            "设备.读取参数",
            "读取设备的当前数值温度、湿度。可以叫获取温度，获取湿度。也可以叫当前温度多少，温湿度多"
            "少。设备名称都是温湿度 目前只支持这一个返回值示例：{\"温度\":26.5,\"湿度\":60}",
            PropertyList({{"传感器名称", kPropertyTypeString}}),
            [this](const PropertyList& params) -> ReturnValue {
                std::string name = params["传感器名称"].value<std::string>();
                HomeDevice* device = FindDevice(name.c_str());
                if (device == nullptr) {
                    return "{\"错误\":\"传感器不存在\"}";
                }

                ESP_LOGI(HOME_MCP_TAG, "读取传感器:%s ID:%d", device->name, device->device_id);

                // 从全局变量读取真实温湿度
                char json_buf[64];
                snprintf(json_buf, sizeof(json_buf), "{\"温度\":%.1f,\"湿度\":%.1f}", g_data.temp,
                         g_data.humi);
                return std::string(json_buf);
            });

        ESP_LOGI(HOME_MCP_TAG, "智能家居MCP注册完成");
    }
};