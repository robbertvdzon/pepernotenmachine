#include "app.h"

#include <Arduino.h>

#include "ble_link.h"
#include "ble_scanner.h"
#include "buttons.h"
#include "config.h"
#include "controller.h"
#include "display.h"
#include "leds.h"
#include "log.h"
#include "mcp23017.h"
#include "rotary_encoder.h"
#include "switches.h"
#include "ui.h"

static const char* TAG = "APP";

// L3 / L5 show the connection state of the launcher / switchboard:
// off = nothing saved, flash = searching or connecting, on = connected, fast flash = error.
static void update_connection_leds() {
    leds_set_state(LED_L3_LAUNCHER_CONNECTION, link_state_led(ble_link_state(DeviceKind::Launcher)));
    leds_set_state(LED_L5_SWITCHBOARD_CONNECTION, link_state_led(ble_link_state(DeviceKind::Switchboard)));
}

void app_init() {
    Serial.begin(115200);
    // USB serial: never block when no PC is attached (e.g. in the tractor).
    Serial.setTxTimeoutMs(0);

    LOGI(TAG, "control unit firmware starting");

    if (!display_init()) LOGE(TAG, "display init failed");
    ui_init();

    rotary_encoder_init();

    if (!mcp.begin()) {
        LOGE(TAG, "MCP23017 not available: LEDs L1-L8 and switches SW1-SW8 disabled, will keep retrying");
    }
    leds_init();
    switches_init();
    buttons_init();

    ble_link_init();  // loads the saved devices and starts keeping them connected
    controller_init();
}

void app_update() {
    const uint32_t now = millis();

    // Hardware polling
    mcp.service(now);
    switches_update(now);
    buttons_update(now);
    ble_scanner_update(now);
    ble_link_update(now);
    update_connection_leds();

    // Input events.
    SwitchEvent sw;
    while (switches_poll_event(sw)) {
        LOGD(TAG, "%s -> %d", switches_name(sw.id), (int)sw.active);
        controller_on_switch(sw);
    }
    ButtonEvent btn;
    while (buttons_poll_event(btn)) {
        LOGD(TAG, "button %s -> %d", buttons_name(btn.id), (int)btn.pressed);
        if (btn.id == ButtonId::Back && btn.pressed) ui_on_back();
    }
    controller_update(now);  // after the switch events above, so a launch press is handled this pass

    ui_on_encoder(rotary_encoder_take_delta());
    ui_on_push(buttons_is_pressed(ButtonId::EncoderPush));

    // UI
    ui_update(now);
    display_update();

    // Yield to the RTOS for one short slice instead of spinning. All timing is
    // millis()-based and non-blocking; LED animation runs in its own task.
    vTaskDelay(pdMS_TO_TICKS(2));
}
