#pragma once

#include "mcp_server.h"
#include <esp_log.h>
#include "led_strip.h"

static const char* WS2812_TAG = "WS2812";

class WS2812_Controller {
private:
    led_strip_handle_t strip_;
    int led_count_;

    void set_pixel_color(int index, uint8_t r, uint8_t g, uint8_t b) {
        if (index >= 0 && index < led_count_) {
            led_strip_set_pixel(strip_, index, r, g, b);
        }
    }

    void refresh() {
        led_strip_refresh(strip_);
    }

public:
    explicit WS2812_Controller(gpio_num_t gpio_num, uint32_t led_count) 
        : led_count_(led_count) {
        
        // ---------- 初始化 LED Strip ----------
        led_strip_config_t strip_config = {
            .strip_gpio_num = gpio_num,
            .max_leds = led_count,
            .led_model = LED_MODEL_WS2812,
            .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
            .flags = {
                .invert_out = false,
            }
        };

        led_strip_rmt_config_t rmt_config = {
            .clk_src = RMT_CLK_SRC_DEFAULT,
            .resolution_hz = 10 * 1000 * 1000,
            .flags = {
                .with_dma = false,
            }
        };
        ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &strip_));
        led_strip_clear(strip_);

        auto& server = McpServer::GetInstance();

        // ---------- 工具1：设置单颗灯珠颜色 ----------
        server.AddTool(
            "灯带.设置颜色",
            "设置指定位置灯珠的RGB颜色",
            PropertyList({
                {"位置", kPropertyTypeInteger},
                {"红色", kPropertyTypeInteger},
                {"绿色", kPropertyTypeInteger},
                {"蓝色", kPropertyTypeInteger}
            }),
            [this](const PropertyList& params) -> ReturnValue {
                int index = params["位置"].value<int>();
                uint8_t r = (uint8_t)params["红色"].value<int>();
                uint8_t g = (uint8_t)params["绿色"].value<int>();
                uint8_t b = (uint8_t)params["蓝色"].value<int>();

                set_pixel_color(index, r, g, b);
                refresh();

                ESP_LOGW(WS2812_TAG, "......yry灯珠 %d 设为 RGB(%d,%d,%d)", index, r, g, b);


                return std::string("{\"状态\":\"灯珠 ") + std::to_string(index) + " 颜色已更新\"}";
            }
        );

        // ---------- 工具2：整条灯带设色 ----------
        server.AddTool(
            "灯带.设置全彩",
            "将整条灯带设置为同一颜色",
            PropertyList({
                {"红色", kPropertyTypeInteger},
                {"绿色", kPropertyTypeInteger},
                {"蓝色", kPropertyTypeInteger}
            }),
            [this](const PropertyList& params) -> ReturnValue {
                uint8_t r = (uint8_t)params["红色"].value<int>();
                uint8_t g = (uint8_t)params["绿色"].value<int>();
                uint8_t b = (uint8_t)params["蓝色"].value<int>();

                for (int i = 0; i < led_count_; ++i) {
                    set_pixel_color(i, r, g, b);
                }
                refresh();

                ESP_LOGW(WS2812_TAG, "......yry灯珠 全彩设置 RGB(%d,%d,%d)", r, g, b);


                return std::string("{\"状态\":\"整条灯带已更新\"}");
            }
        );

        // ---------- 工具3：关闭灯带 ----------
        server.AddTool(
            "灯带.关闭",
            "关闭整条灯带，所有灯珠熄灭",
            PropertyList(),
            [this](const PropertyList&) -> ReturnValue {
                led_strip_clear(strip_);
                ESP_LOGW(WS2812_TAG, "灯带已关闭");
                return true;
            }
        );
    }

    // ---------- 供外部调用的公开方法 ----------
    void setPixel(int index, uint8_t r, uint8_t g, uint8_t b) {
        set_pixel_color(index, r, g, b);
    }

    void setPixelAndRefresh(int index, uint8_t r, uint8_t g, uint8_t b) {
        set_pixel_color(index, r, g, b);
        refresh();
    }

    void setAll(uint8_t r, uint8_t g, uint8_t b) {
        for (int i = 0; i < led_count_; ++i) {
            set_pixel_color(i, r, g, b);
        }
        refresh();
    }

    void clear() {
        led_strip_clear(strip_);
    }

    void show() {
        refresh();
    }

    int getLedCount() const { return led_count_; }
};