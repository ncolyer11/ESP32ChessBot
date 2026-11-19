#pragma once

// Small hooks for things that differ between the desktop build and the ESP32 build
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Let the idle task run now and then during long searches (no-op for most calls)
inline void platform_yield() {
    static int64_t last_us = 0;
    int64_t now = esp_timer_get_time();
    if (now - last_us >= 100000) {
        last_us = now;
        vTaskDelay(1);
    }
}

inline uint32_t platform_random_seed() { return esp_random(); }

// Transposition table: 2^17 entries (~1.5 MB) in PSRAM if there is any, else 2^12 (48 KB) in internal ram
inline int platform_tt_bits() { return heap_caps_get_total_size(MALLOC_CAP_SPIRAM) > 0 ? 17 : 12; }

inline void *platform_alloc_tt(size_t bytes) {
    void *p = nullptr;
    if (heap_caps_get_total_size(MALLOC_CAP_SPIRAM) > 0) p = heap_caps_calloc(1, bytes, MALLOC_CAP_SPIRAM);
    return p ? p : heap_caps_calloc(1, bytes, MALLOC_CAP_8BIT);
}
#else
inline void platform_yield() {}
inline uint32_t platform_random_seed() { return static_cast<uint32_t>(std::time(nullptr)); }
inline int platform_tt_bits() { return 16; }
inline void *platform_alloc_tt(size_t bytes) { return std::calloc(1, bytes); }
#endif
