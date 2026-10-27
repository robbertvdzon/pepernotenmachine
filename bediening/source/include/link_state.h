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

// Short texts for the 160 px display (<= 17 characters at 8 px per character).
inline const char* link_state_text(LinkState s) {
    switch (s) {
        case LinkState::NotConfigured: return "not set";
        case LinkState::Searching: return "searching";
        case LinkState::Connecting: return "connecting";
        case LinkState::Connected: return "connected";
        case LinkState::Error: return "error";
    }
    return "?";
}

// Error details (also <= 17 characters).
#define LINK_ERR_NONE ""
#define LINK_ERR_SERVICE_MISSING "service missing"
#define LINK_ERR_CHAR_MISSING "characteristic?"
#define LINK_ERR_NOT_WRITABLE "not writable"
#define LINK_ERR_SUBSCRIBE "subscribe failed"
#define LINK_ERR_READ "read failed"
#define LINK_ERR_NO_CLIENT "no BLE client"
