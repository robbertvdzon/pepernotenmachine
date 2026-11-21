#pragma once

// Pure quadrature decoder (host-testable). Same transition table as the
// original test project: 4 valid transitions = 1 detent. Invalid transitions
// (contact bounce) contribute 0.

#include <stdint.h>

class QuadratureDecoder {
   public:
    void reset(uint8_t state) {
        prev_ = state & 0x03;
        sum_ = 0;
    }

    // `state` = (CLK << 1) | CT. Returns +1 / -1 when a full detent completed, else 0.
    int8_t step(uint8_t state) {
        static const int8_t table[16] = {
            0, -1, 1, 0,
            1, 0, 0, -1,
            -1, 0, 0, 1,
            0, 1, -1, 0,
        };
        state &= 0x03;
        sum_ += table[(prev_ << 2) | state];
        prev_ = state;
        if (sum_ >= 4) {
            sum_ = 0;
            return 1;
        }
        if (sum_ <= -4) {
            sum_ = 0;
            return -1;
        }
        return 0;
    }

   private:
    uint8_t prev_ = 0;
    int8_t sum_ = 0;
};
