#pragma once

// Persistent record for one selected device (host-testable encode/decode).
// Layout: [0]=version [1]=addrType [2..7]=address (display order)
//         [8]=nameLen [9..]=name (no NUL). Anything that does not validate
// decodes to "no device" instead of garbage.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

constexpr size_t DEVICE_NAME_MAX = 23;  // + NUL = 24
constexpr uint8_t DEVICE_RECORD_VERSION = 1;
constexpr size_t DEVICE_RECORD_HEADER = 9;
constexpr size_t DEVICE_RECORD_MAX = DEVICE_RECORD_HEADER + DEVICE_NAME_MAX;

struct StoredDevice {
    bool valid = false;
    uint8_t addr[6] = {0, 0, 0, 0, 0, 0};
    uint8_t addrType = 0;
    char name[DEVICE_NAME_MAX + 1] = {0};
};

// Returns bytes written, or 0 if the device is invalid / the buffer too small.
inline size_t device_record_encode(const StoredDevice& d, uint8_t* buf, size_t cap) {
    if (!d.valid) return 0;
    size_t nlen = 0;
    while (nlen < DEVICE_NAME_MAX && d.name[nlen]) nlen++;
    const size_t total = DEVICE_RECORD_HEADER + nlen;
    if (cap < total) return 0;
    buf[0] = DEVICE_RECORD_VERSION;
    buf[1] = d.addrType;
    memcpy(buf + 2, d.addr, 6);
    buf[8] = static_cast<uint8_t>(nlen);
    memcpy(buf + DEVICE_RECORD_HEADER, d.name, nlen);
    return total;
}

inline bool device_record_decode(const uint8_t* buf, size_t len, StoredDevice& out) {
    out = StoredDevice();
    if (!buf || len < DEVICE_RECORD_HEADER) return false;
    if (buf[0] != DEVICE_RECORD_VERSION) return false;
    if (buf[1] > 3) return false;
    const size_t nlen = buf[8];
    if (nlen > DEVICE_NAME_MAX || len != DEVICE_RECORD_HEADER + nlen) return false;
    bool all_zero = true;
    for (int i = 0; i < 6; i++)
        if (buf[2 + i]) all_zero = false;
    if (all_zero) return false;

    out.valid = true;
    out.addrType = buf[1];
    memcpy(out.addr, buf + 2, 6);
    memcpy(out.name, buf + DEVICE_RECORD_HEADER, nlen);
    out.name[nlen] = '\0';
    return true;
}
