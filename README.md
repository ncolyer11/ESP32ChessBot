# ESP32ChessBot
A ~1-1.5k elo chess bot written in C++ designed for speed but mainly memory efficiency so it can run on an ESP32 with only ~200kB of ram.


## Usage
Build with `make` (Linux, macOS, WSL or Windows with MinGW), the binary is `bin/chesp32` (`bin/chesp32.exe` on Windows).

    bin/chesp32 [--mode hvb|bvh|hvh|bvb] [--depth N] [--time MS] [--fen "<fen>"]
    bin/chesp32 --perft N [--fen "<fen>"]
    bin/chesp32 --bench [--depth N]

Add `--ascii` for plain letters instead of unicode pieces.

By default you are asked whether to play white, black, two players or bots only. The bot searches to `--depth`
(default 6) and optionally stops after `--time` milliseconds per move.

In game, type moves like `e2e4` (`e7e8q` to promote, queen by default) or a command: `u` undo to your last turn,
`z` undo 2 plies, `f` print FEN, `r` resign, `q` quit, `h` help.

`make test` runs the tests in `tests/` (each `.in` is fed to `--test` mode and diffed against the `.out`),
`make perft` checks move generation against known perft numbers. Build with `-DCHESS_DEBUG` to verify the
incremental zobrist hash after every move.


## ESP32-S3
The same engine runs on an ESP32-S3 (tested target: ESP32-S3-DevKitC-1, with or without PSRAM) and is played over
the USB serial console. It is built with [PlatformIO](https://platformio.org/) and ESP-IDF:

    pio run -e esp32s3                       # build
    pio run -e esp32s3 -t upload -t monitor  # flash and open the console (115200 baud)

Plug into the port labelled `UART` on the devkit (the CP210x/CH340 one), PlatformIO normally finds it by itself,
otherwise add `--upload-port /dev/ttyUSB0` (`COM3` on Windows). Reset the board after flashing if the menu
doesn't show up. Exit the monitor with ctrl + c.

The menu asks what to do (play, bench, perft suite), which side you play, search depth (default 8, max 16),
time per move in ms (default 5000), an optional FEN and unicode or ascii board. Moves and commands are the same as on
the desktop. After each game or bench it prints free heap, PSRAM, transposition table size and the stack
high-water mark of the engine task.

With 8 MB octal PSRAM (N8R8, N16R8) the transposition table is 2^17 entries (~1.5 MB) in PSRAM, without PSRAM
it falls back to 2^12 entries (48 KB) in internal ram. Quad PSRAM boards (e.g. N8R2) need
`CONFIG_SPIRAM_MODE_QUAD=y` instead of `CONFIG_SPIRAM_MODE_OCT=y` in `sdkconfig.defaults`.

To use the native USB port instead of the UART one, replace `CONFIG_ESP_CONSOLE_UART_DEFAULT=y` with
`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` in `sdkconfig.defaults` (or run `pio run -e esp32s3 -t menuconfig`),
delete the generated `sdkconfig.esp32s3` and rebuild.
