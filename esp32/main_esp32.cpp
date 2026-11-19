// ESP32-S3 entry point: serial console menu, then the shared game loop in src/app.cpp
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "driver/uart.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "driver/uart_vfs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#endif

#include "app.h"
#include "chess_search.h"
#include "util.h"

constexpr int ENGINE_STACK_BYTES = 48 * 1024;  // search recursion is ~1.3 KB per ply
constexpr int ENGINE_CORE = 1;
constexpr int MENU_DEFAULT_DEPTH = 8;
constexpr int MENU_DEFAULT_TIME_MS = 5000;

// Get stdin/stdout talking to the console with proper blocking reads and line endings
static void console_init() {
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    usb_serial_jtag_driver_install(&cfg);
    usb_serial_jtag_vfs_use_driver();
    usb_serial_jtag_vfs_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    usb_serial_jtag_vfs_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
#else
    const uart_port_t port = (uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM;
    uart_driver_install(port, 256, 0, 0, nullptr, 0);
    uart_vfs_dev_use_driver(port);
    uart_vfs_dev_port_set_rx_line_endings(port, ESP_LINE_ENDINGS_CR); // enter sends CR (or LF)
    uart_vfs_dev_port_set_tx_line_endings(port, ESP_LINE_ENDINGS_CRLF);
#endif
    setvbuf(stdin, nullptr, _IONBF, 0);
}

// Prompt and read one line, returns "" on empty input or EOF
static std::string ask(const char *prompt) {
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) {
        std::cin.clear();
        return "";
    }
    normalise_line(line);
    return line;
}

static int ask_int(const char *prompt, int def, int lo, int hi) {
    std::string s = ask(prompt);
    if (s.empty()) return def;
    char *end = nullptr;
    long v = std::strtol(s.c_str(), &end, 10);
    if (end == s.c_str()) return def;
    return (int)std::max<long>(lo, std::min<long>(hi, v));
}

static void print_stats() {
    std::cout << "\n-- memory --\n"
              << "free heap: " << esp_get_free_heap_size() << " B (min ever " << esp_get_minimum_free_heap_size() << ")\n"
              << "internal free: " << heap_caps_get_free_size(MALLOC_CAP_INTERNAL) << " B\n"
              << "psram: " << heap_caps_get_total_size(MALLOC_CAP_SPIRAM) << " B total, "
              << heap_caps_get_free_size(MALLOC_CAP_SPIRAM) << " B free\n"
              << "tt: " << tt_bytes() << " B\n"
              << "engine stack high water mark: " << uxTaskGetStackHighWaterMark(nullptr) << " B used of "
              << ENGINE_STACK_BYTES << " B (stack left, lowest ever)\n" << std::endl;
}

static void engine_task(void *) {
    app_init();
    std::cout << "\nChesp32 on ESP32-S3, psram " << heap_caps_get_total_size(MALLOC_CAP_SPIRAM)
              << " B, tt " << tt_bytes() << " B" << std::endl;

    while (true) {
        AppConfig cfg;
        std::string what = ask("\n(p)lay, (b)ench, (t) perft suite? [p] ");
        char c = what.empty() ? 'p' : (char)tolower(what[0]);

        if (c == 't') {
            cfg.perft_suite = true;
        } else {
            if (c == 'b') {
                cfg.bench = true;
            } else {
                std::string side = ask("Play as (w)hite, (b)lack, (2) players or (0) bots-only? [w] ");
                switch (side.empty() ? 'w' : (char)tolower(side[0])) {
                    case 'b': cfg.mode = Mode::BVH; break;
                    case '2': cfg.mode = Mode::HVH; break;
                    case '0': cfg.mode = Mode::BVB; break;
                    default: cfg.mode = Mode::HVB; break;
                }
                cfg.fen = ask("Start FEN (empty = standard start): ");
            }
            cfg.depth = ask_int("Bot depth 1-16? [8] ", MENU_DEFAULT_DEPTH, 1, 16);
            cfg.time_ms = ask_int("Bot time per move in ms, 0 = no limit? [5000] ", MENU_DEFAULT_TIME_MS, 0, 600000);
            std::string board = ask("Board (u)nicode or (a)scii? [u] ");
            cfg.ascii_board = !board.empty() && tolower(board[0]) == 'a';
        }

        int rc = app_run(cfg);
        std::cout << "\nFinished (exit code " << rc << ")" << std::endl;
        print_stats();
        ask("Press enter for the menu ");
    }
}

extern "C" void app_main() {
    console_init();
    xTaskCreatePinnedToCore(engine_task, "engine", ENGINE_STACK_BYTES, nullptr, 5, nullptr, ENGINE_CORE);
}
