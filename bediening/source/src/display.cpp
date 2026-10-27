#include "display.h"

#include "config.h"
#include "log.h"

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lvgl.h>

static const char* TAG = "DISP";

// =========================================================================
// LOVYANGFX HARDWARE CONFIGURATION (ST7735S + Arduino Nano ESP32)
// Unchanged from the working test project; only rotation is applied later.
// =========================================================================
class LGFX_NanoESP32_ST7735 : public lgfx::LGFX_Device {
    lgfx::Panel_ST7735S _panel_instance;
    lgfx::Bus_SPI _bus_instance;

   public:
    LGFX_NanoESP32_ST7735() {
        // SPI bus config (official AZ-Delivery pinout)
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST;  // hardware SPI of the ESP32-S3
            cfg.spi_mode = 0;
            cfg.freq_write = DISPLAY_SPI_HZ;  // ST7735S may have problems at higher speeds
            cfg.pin_sclk = TFT_PIN_SCLK;
            cfg.pin_mosi = TFT_PIN_MOSI;
            cfg.pin_dc = TFT_PIN_DC;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        // Panel config (the display itself; native portrait 128x160)
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs = TFT_PIN_CS;
            cfg.pin_rst = TFT_PIN_RST;
            cfg.panel_width = 128;
            cfg.panel_height = 160;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.invert = DISPLAY_INVERT;
            cfg.rgb_order = DISPLAY_RGB_ORDER;
            _panel_instance.config(cfg);
        }

        setPanel(&_panel_instance);
    }
};

static LGFX_NanoESP32_ST7735 tft;

// LVGL renders into a full-frame RGB565 buffer ("direct" mode): every changed region is
// drawn in one go and only the changed rows are sent to the panel. The stride is rounded
// up to 64 bytes, which covers the alignment rule of every LVGL version.
constexpr uint32_t FRAME_STRIDE_MAX = ((DISPLAY_WIDTH * 2 + 63) / 64) * 64;
alignas(64) static uint8_t frame_buffer[FRAME_STRIDE_MAX * DISPLAY_HEIGHT];
static uint32_t frame_stride = FRAME_STRIDE_MAX;

static void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    const uint32_t w = area->x2 - area->x1 + 1;

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, area->y2 - area->y1 + 1);
    for (int32_t y = area->y1; y <= area->y2; y++) {
        const uint8_t* row = px_map + static_cast<uint32_t>(y) * frame_stride;
        // LVGL's RGB565 is native-endian = lgfx::rgb565_t. A plain uint16_t* would be taken as
        // byte-swapped data (TFT_eSPI convention) and every color would come out wrong.
        tft.writePixels(reinterpret_cast<const lgfx::rgb565_t*>(row) + area->x1, w);
    }
    tft.endWrite();

    lv_display_flush_ready(disp);
}

bool display_init() {
    const bool ok = tft.init();
    tft.setRotation(DISPLAY_ROTATION);
    tft.clear(TFT_BLACK);

    lv_init();
    lv_tick_set_cb([]() -> uint32_t { return millis(); });  // LVGL needs a millisecond clock

    lv_display_t* disp = lv_display_create(tft.width(), tft.height());
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    frame_stride = lv_draw_buf_width_to_stride(tft.width(), LV_COLOR_FORMAT_RGB565);
    if (frame_stride * tft.height() > sizeof(frame_buffer)) {
        LOGE(TAG, "frame buffer too small for LVGL stride %lu", (unsigned long)frame_stride);
        return false;
    }
    lv_display_set_buffers(disp, frame_buffer, NULL, sizeof(frame_buffer), LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(disp, flush_cb);

    LOGI(TAG, "ST7735S %dx%d, rotation %u, SPI %lu Hz, invert=%d, rgb_order=%d", (int)tft.width(), (int)tft.height(),
         (unsigned)DISPLAY_ROTATION, (unsigned long)DISPLAY_SPI_HZ, (int)DISPLAY_INVERT, (int)DISPLAY_RGB_ORDER);
    return ok;
}

uint32_t display_update() { return lv_timer_handler(); }
