#include "buttons.h"

#include "config.h"
#include "debounce.h"
#include "event_queue.h"
#include "log.h"
#include "switches.h"

static const char* TAG = "BTN";

static Debouncer push_debouncer;
static bool back_pressed = false;
static bool back_seeded = false;  // first valid SW1 reading seen
static EventQueue<ButtonEvent, 8> events;

static inline bool read_push_raw() { return digitalRead(ROTARY_PIN_SWITCH) == LOW; }

void buttons_init() {
    pinMode(ROTARY_PIN_SWITCH, INPUT_PULLUP);
    push_debouncer.reset(read_push_raw(), millis());
    back_pressed = false;
    back_seeded = false;
}

void buttons_update(uint32_t now) {
    if (push_debouncer.update(read_push_raw(), now, BUTTON_DEBOUNCE_MS)) {
        events.push(ButtonEvent{ButtonId::EncoderPush, push_debouncer.state(), now});
    }

    // SW1 is already debounced by switches.cpp; just turn it into button events.
    if (switches_valid()) {
        const bool now_pressed = switches_is_active(SW1_BACK);
        if (!back_seeded) {
            back_seeded = true;  // do not report a button that is held at boot
            back_pressed = now_pressed;
        } else if (now_pressed != back_pressed) {
            back_pressed = now_pressed;
            events.push(ButtonEvent{ButtonId::Back, now_pressed, now});
        }
    }

    if (events.dropped() > 0) {
        static uint32_t reported = 0;
        if (events.dropped() != reported) {
            reported = events.dropped();
            LOGW(TAG, "event queue overflow, %lu events dropped in total", (unsigned long)reported);
        }
    }
}

bool buttons_is_pressed(ButtonId id) {
    return id == ButtonId::EncoderPush ? push_debouncer.state() : back_pressed;
}

bool buttons_poll_event(ButtonEvent& out) { return events.pop(out); }

const char* buttons_name(ButtonId id) { return id == ButtonId::EncoderPush ? "Encoder push" : "Back"; }
