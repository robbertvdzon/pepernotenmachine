#pragma once

// Connection state of one remote device (pure, host-testable).
//
//   NotConfigured  no device saved                 -> LED off
//   Searching      saved, not connected (out of range / not powered / retrying)
//                                                  -> LED flash
//   Connecting     link is up, discovering services, subscribing, first reads
//                                                  -> LED flash
//   Connected      link up AND all initial state read; values are current
//                                                  -> LED on
//   Error          link reachable but unusable (wrong/missing service or
//                  characteristic, setup failed); retried with a back-off
//                                                  -> LED fast flash

#include <stdint.h>

#include "led_pattern.h"
#include "ui_strings.h"

enum class LinkState : uint8_t { NotConfigured = 0, Searching, Connecting, Connected, Error };

inline const char* link_state_name(LinkState s) {
    switch (s) {
        case LinkState::NotConfigured: return "NotConfigured";
        case LinkState::Searching: return "Searching";
        case LinkState::Connecting: return "Connecting";
        case LinkState::Connected: return "Connected";
        case LinkState::Error: return "Error";
    }
    return "?";
}

inline led_state_t link_state_led(LinkState s) {
    switch (s) {
        case LinkState::NotConfigured: return LED_OFF;
        case LinkState::Searching: return LED_FLASH;
        case LinkState::Connecting: return LED_FLASH;
        case LinkState::Connected: return LED_ON;
        case LinkState::Error: return LED_FLASH_FAST;
    }
    return LED_OFF;
}

// Short texts for the 160 px display (<= 17 characters at 8 px per character; see ui_strings_en.h).
inline const char* link_state_text(LinkState s) {
    switch (s) {
        case LinkState::NotConfigured: return STR_LINK_NOT_SET;
        case LinkState::Searching: return STR_LINK_SEARCHING;
        case LinkState::Connecting: return STR_LINK_CONNECTING;
        case LinkState::Connected: return STR_LINK_CONNECTED;
        case LinkState::Error: return STR_LINK_ERROR;
    }
    return "?";
}

// Error details (texts live in ui_strings_en.h).
#define LINK_ERR_NONE ""
#define LINK_ERR_SERVICE_MISSING STR_ERR_SERVICE_MISSING
#define LINK_ERR_CHAR_MISSING STR_ERR_CHAR_MISSING
#define LINK_ERR_NOT_WRITABLE STR_ERR_NOT_WRITABLE
#define LINK_ERR_SUBSCRIBE STR_ERR_SUBSCRIBE
#define LINK_ERR_READ STR_ERR_READ
#define LINK_ERR_NO_CLIENT STR_ERR_NO_CLIENT
