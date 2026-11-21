#pragma once

// Tiny fixed-size FIFO for events produced and consumed in the same context
// (the main loop). Not ISR-safe. When full, new events are dropped and counted
// so the loss is visible.

#include <stddef.h>
#include <stdint.h>

template <typename T, size_t N>
class EventQueue {
   public:
    bool push(const T& e) {
        if (count_ == N) {
            dropped_++;
            return false;
        }
        buf_[(head_ + count_) % N] = e;
        count_++;
        return true;
    }

    bool pop(T& out) {
        if (count_ == 0) return false;
        out = buf_[head_];
        head_ = (head_ + 1) % N;
        count_--;
        return true;
    }

    size_t size() const { return count_; }
    uint32_t dropped() const { return dropped_; }

   private:
    T buf_[N];
    size_t head_ = 0;
    size_t count_ = 0;
    uint32_t dropped_ = 0;
};
