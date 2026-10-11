#include <Arduino.h>
#include <Preferences.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "../include/storage.h"

#define STORAGE_NAMESPACE "launcher"
#define STORAGE_WRITE_QUEUE_LENGTH 8

typedef struct {
    char key[STORAGE_MAX_KEY_LENGTH + 1];
    size_t valueSize;
    uint8_t value[STORAGE_MAX_VALUE_SIZE];
} storage_write_t;

static QueueHandle_t storageWriteQueue = NULL;

static bool valid_key(const char* key) {
    if (key == NULL || key[0] == '\0') return false;

    size_t length = 0;
    while (length <= STORAGE_MAX_KEY_LENGTH && key[length] != '\0') {
        ++length;
    }
    return length > 0 && length <= STORAGE_MAX_KEY_LENGTH;
}

static void storage_writer_task(void* arg) {
    storage_write_t write;
    (void)arg;

    for (;;) {
        if (xQueueReceive(storageWriteQueue, &write, portMAX_DELAY) == pdTRUE) {
            Preferences preferences;
            if (preferences.begin(STORAGE_NAMESPACE, false)) {
                preferences.putBytes(write.key, write.value, write.valueSize);
                preferences.end();
            }
        }
    }
}

bool storage_init(void) {
    if (storageWriteQueue != NULL) return true;

    storageWriteQueue = xQueueCreate(STORAGE_WRITE_QUEUE_LENGTH, sizeof(storage_write_t));
    if (storageWriteQueue == NULL) return false;

    if (xTaskCreate(storage_writer_task, "storageWriter", 3072, NULL, 1, NULL) != pdPASS) {
        vQueueDelete(storageWriteQueue);
        storageWriteQueue = NULL;
        return false;
    }

    return true;
}

bool storage_get(const char* key, void* value, size_t capacity, size_t* valueSize) {
    if (!valid_key(key) || value == NULL) return false;

    Preferences preferences;
    if (!preferences.begin(STORAGE_NAMESPACE, true)) return false;

    const size_t storedSize = preferences.getBytesLength(key);
    if (valueSize != NULL) *valueSize = storedSize;
    const bool read = storedSize > 0 && storedSize <= capacity &&
                      preferences.getBytes(key, value, storedSize) == storedSize;
    preferences.end();
    return read;
}

bool storage_set(const char* key, const void* value, size_t valueSize) {
    if (!valid_key(key) || value == NULL || valueSize == 0 || valueSize > STORAGE_MAX_VALUE_SIZE ||
        storageWriteQueue == NULL) {
        return false;
    }

    storage_write_t write = {};
    size_t keyLength = 0;
    while (key[keyLength] != '\0') ++keyLength;
    memcpy(write.key, key, keyLength + 1);
    memcpy(write.value, value, valueSize);
    write.valueSize = valueSize;

    return xQueueSend(storageWriteQueue, &write, 0) == pdTRUE;
}