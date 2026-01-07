#include "../include/led.h"
#include "../include/config.h"
#include "../include/ble_manager.h"
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <math.h>

// single pixel
static Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);

static volatile uint8_t mode = LED_MODE_OFF;
static volatile uint8_t baseR = 0, baseG = 0, baseB = 0;

// runtime state
static float curR = 0, curG = 0, curB = 0;

// flash timing
static const uint32_t FLASH_MS = 150;
static uint32_t lastFlashToggle = 0;
static bool flashOn = false;

// sine pulsing
static const float SINE_FREQ_HZ = 1.0f; // 1 Hz pulse

// rainbow
static const uint32_t RAINBOW_PERIOD_MS = 5000; // 5s full cycle

// crossfade
static bool inCrossfade = false;
static uint8_t cf_startR=0, cf_startG=0, cf_startB=0;
static uint8_t cf_targetR=0, cf_targetG=0, cf_targetB=0;
static uint32_t cf_startTime = 0;
static uint32_t cf_duration = 0; // ms

static uint32_t lastUpdate = 0;

static void applyColor(float r, float g, float b) {
    // clamp and write to pixel
    uint8_t R = (uint8_t)constrain(roundf(r), 0, 255);
    uint8_t G = (uint8_t)constrain(roundf(g), 0, 255);
    uint8_t B = (uint8_t)constrain(roundf(b), 0, 255);
    strip.setPixelColor(0, strip.Color(R, G, B));
    strip.show();
}

static void hsv_to_rgb(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b) {
    // h in [0,360), s,v in [0,1]
    float c = v * s;
    float hh = h / 60.0f;
    float x = c * (1 - fabsf(fmodf(hh, 2.0f) - 1));
    float m = v - c;
    float rr=0, gg=0, bb=0;
    if (hh >= 0 && hh < 1) { rr=c; gg=x; bb=0; }
    else if (hh < 2) { rr=x; gg=c; bb=0; }
    else if (hh < 3) { rr=0; gg=c; bb=x; }
    else if (hh < 4) { rr=0; gg=x; bb=c; }
    else if (hh < 5) { rr=x; gg=0; bb=c; }
    else { rr=c; gg=0; bb=x; }
    r = (uint8_t)constrain(roundf((rr + m)*255.0f), 0, 255);
    g = (uint8_t)constrain(roundf((gg + m)*255.0f), 0, 255);
    b = (uint8_t)constrain(roundf((bb + m)*255.0f), 0, 255);
}

void led_init() {
    strip.begin();
    strip.show();
    mode = LED_MODE_OFF;
    baseR = baseG = baseB = 0;
    curR = curG = curB = 0;
    inCrossfade = false;
}

void led_set(uint8_t m, uint8_t r, uint8_t g, uint8_t b) {
    // called from BLE task potentially
    mode = m;
    baseR = r; baseG = g; baseB = b;
    inCrossfade = false; // cancel any running crossfade
    // set immediate color depending on mode
    if (mode == LED_MODE_OFF) {
        curR = curG = curB = 0;
        applyColor(0,0,0);
    } else if (mode == LED_MODE_ON) {
        curR = baseR; curG = baseG; curB = baseB;
        applyColor(curR, curG, curB);
    } else if (mode == LED_MODE_FLASH) {
        lastFlashToggle = millis();
        flashOn = true;
        applyColor(baseR, baseG, baseB);
    } else if (mode == LED_MODE_SINE) {
        // start with mid brightness
        curR = baseR; curG = baseG; curB = baseB;
    } else if (mode == LED_MODE_RAINBOW) {
        // nothing immediate
    }
}

void led_set_from_bytes(const uint8_t* data, size_t len) {
    if (len < 4) return;
    uint8_t m = data[0];
    uint8_t r = data[1];
    uint8_t g = data[2];
    uint8_t b = data[3];
    if (m == LED_MODE_CROSSFADE) {
        // expect extra 2 bytes for duration (ms), big-endian
        if (len < 6) return;
        uint16_t dur = ((uint16_t)data[4] << 8) | (uint16_t)data[5];
        // start crossfade from current color to target
        cf_startR = (uint8_t)constrain(roundf(curR), 0, 255);
        cf_startG = (uint8_t)constrain(roundf(curG), 0, 255);
        cf_startB = (uint8_t)constrain(roundf(curB), 0, 255);
        cf_targetR = r; cf_targetG = g; cf_targetB = b;
        cf_startTime = millis();
        cf_duration = dur;
        inCrossfade = true;
        // set mode to crossfade while preserving base color
        mode = LED_MODE_CROSSFADE;
        baseR = r; baseG = g; baseB = b;
    } else {
        led_set(m, r, g, b);
    }
}

void led_get_state(uint8_t out[4]) {
    out[0] = mode;
    out[1] = baseR;
    out[2] = baseG;
    out[3] = baseB;
}

void led_update() {
    uint32_t now = millis();
    // limit update rate
    if (now - lastUpdate < 16) return; // ~60Hz
    lastUpdate = now;

    if (inCrossfade) {
        if (cf_duration == 0) {
            // immediate
            curR = cf_targetR; curG = cf_targetG; curB = cf_targetB;
            inCrossfade = false;
            mode = LED_MODE_ON;
            baseR = cf_targetR; baseG = cf_targetG; baseB = cf_targetB;
            applyColor(curR, curG, curB);
            // notify client crossfade done
            ble_notify_led_done();
            return;
        }
        uint32_t elapsed = now - cf_startTime;
        if (elapsed >= cf_duration) {
            curR = cf_targetR; curG = cf_targetG; curB = cf_targetB;
            inCrossfade = false;
            mode = LED_MODE_ON;
            baseR = cf_targetR; baseG = cf_targetG; baseB = cf_targetB;
            applyColor(curR, curG, curB);
            // notify client crossfade done
            ble_notify_led_done();
            return;
        }
        float t = (float)elapsed / (float)cf_duration;
        curR = (1.0f - t) * cf_startR + t * cf_targetR;
        curG = (1.0f - t) * cf_startG + t * cf_targetG;
        curB = (1.0f - t) * cf_startB + t * cf_targetB;
        applyColor(curR, curG, curB);
        return;
    }

    switch (mode) {
        case LED_MODE_OFF:
            // ensure off
            applyColor(0,0,0);
            break;
        case LED_MODE_ON:
            applyColor(baseR, baseG, baseB);
            break;
        case LED_MODE_FLASH: {
            if ((now - lastFlashToggle) >= FLASH_MS) {
                lastFlashToggle = now;
                flashOn = !flashOn;
            }
            if (flashOn) applyColor(baseR, baseG, baseB);
            else applyColor(0,0,0);
            break;
        }
        case LED_MODE_SINE: {
            float phase = (float)(now % 1000000) / 1000.0f; // ms to seconds
            float t = phase * SINE_FREQ_HZ * 2.0f * 3.14159265f;
            float brightness = (sinf(t) + 1.0f) / 2.0f; // 0..1
            float r = baseR * brightness;
            float g = baseG * brightness;
            float b = baseB * brightness;
            applyColor(r,g,b);
            break;
        }
        case LED_MODE_RAINBOW: {
            uint32_t pos = now % RAINBOW_PERIOD_MS;
            float hue = (360.0f * pos) / (float)RAINBOW_PERIOD_MS; // 0..360
            uint8_t r,g,b;
            hsv_to_rgb(hue, 1.0f, 1.0f, r, g, b);
            applyColor(r,g,b);
            break;
        }
        default:
            // unknown -> off
            applyColor(0,0,0);
            break;
    }
}
