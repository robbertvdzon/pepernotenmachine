#pragma once

// BLE address helpers (host-testable). Addresses are kept in DISPLAY order
// (most significant byte first, as printed "08:D1:F9:E6:C4:06"). NimBLE's
// getNative() is little-endian, so convert with ble_address_from_native().

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

inline void ble_address_from_native(const uint8_t native[6], uint8_t out[6]) {
    for (int i = 0; i < 6; i++) out[i] = native[5 - i];
}

inline bool ble_address_equal(const uint8_t a[6], const uint8_t b[6]) { return memcmp(a, b, 6) == 0; }

inline bool ble_address_is_zero(const uint8_t a[6]) {
    for (int i = 0; i < 6; i++)
        if (a[i]) return false;
    return true;
}

// Writes "AA:BB:CC:DD:EE:FF" (needs cap >= 18).
inline bool ble_address_format(const uint8_t a[6], char* out, size_t cap) {
    if (cap < 18) return false;
    snprintf(out, cap, "%02X:%02X:%02X:%02X:%02X:%02X", a[0], a[1], a[2], a[3], a[4], a[5]);
    return true;
}

inline int ble_hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

// Strict parser: exactly "XX:XX:XX:XX:XX:XX".
inline bool ble_address_parse(const char* s, uint8_t out[6]) {
    if (!s || strlen(s) != 17) return false;
    uint8_t tmp[6];
    for (int i = 0; i < 6; i++) {
        const int hi = ble_hex_value(s[i * 3]);
        const int lo = ble_hex_value(s[i * 3 + 1]);
        if (hi < 0 || lo < 0) return false;
        if (i < 5 && s[i * 3 + 2] != ':') return false;
        tmp[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
    memcpy(out, tmp, 6);
    return true;
}

// 0 public, 1 random, 2 public-identity (resolved), 3 random-identity (resolved)
inline const char* ble_address_type_name(uint8_t t) {
    switch (t) {
        case 0: return "public";
        case 1: return "random";
        case 2: return "public-id";
        case 3: return "random-id";
        default: return "?";
    }
}
