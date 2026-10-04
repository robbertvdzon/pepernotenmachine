#pragma once

#include <stdbool.h>
#include <stddef.h>

#define STORAGE_MAX_KEY_LENGTH 15
#define STORAGE_MAX_VALUE_SIZE 64
#define STORAGE_KEY_DISPENSER_DURATION "dispDuration"

// Initialize persistent storage and its asynchronous write worker.
bool storage_init(void);

// Read or queue a value by key. Values are opaque bytes, up to STORAGE_MAX_VALUE_SIZE.
bool storage_get(const char* key, void* value, size_t capacity, size_t* valueSize);
bool storage_set(const char* key, const void* value, size_t valueSize);