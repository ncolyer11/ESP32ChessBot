# ESP32ChessBot
A ~2000 (Lichess)/~1800 (FIDE) ELO chess bot written in C++, designed for speed but mainly memory efficiency so it can run on an
ESP32-S3. It builds as a normal desktop program too (Linux, macOS, WSL, Windows).

The bot beating Lichess level 5 (~2k ELO):
<img width="4998" height="1200" alt="image" src="https://github.com/user-attachments/assets/cfdce309-9eba-45cf-8fe2-0df782e232cc" />


## Usage
Build with `make` (Linux, macOS, WSL or Windows with MinGW), the binary is `bin/chesp32` (`bin/chesp32.exe` on Windows).

    bin/chesp32 [--mode hvb|bvh|hvh|bvb] [--depth N] [--time MS] [--fen "<fen>"] [--ascii]
    bin/chesp32 --perft N [--fen "<fen>"]
    bin/chesp32 --perft-suite
    bin/chesp32 --bench [--depth N]
    bin/chesp32 --help

| Flag | What it does |
|---|---|
| `--mode` | who plays each side: `hvb` human white vs bot, `bvh`, `hvh`, `bvb` (default: asks) |
| `--depth N` | bot max search depth (default 6) |
| `--time MS` | bot time limit per move in ms (default 0 = no limit, depth only) |
| `--fen "<fen>"` | start from this position instead of the standard start |
| `--ascii` | plain letters instead of unicode pieces, for terminals without unicode |
| `--perft N` | count move-tree leaves to depth N (move generator check) |
| `--perft-suite` | run known perft positions, non-zero exit on a mismatch |
| `--bench` | time the bot on a few positions at `--depth` |
| `--test` | deterministic test mode used by `make test` (ascii board, FEN and players read from stdin) |

By default you are asked whether to play white, black, two players or bots only.

In game, type moves like `e2e4` (`e7e8q` to promote, queen by default) or a command:

| Cmd | |
|---|---|
| `u` | undo back to your last turn |
| `z` | undo 2 plies |
| `f` | print the FEN |
| `d` | print the board from the bitboards |
| `p` | print side to move, castling, en passant, clocks |
| `b` | print raw bitboards |
| `r` | resign |
| `q` | quit |
| `h` | help |

After each bot move it prints `info depth .. nodes .. time ..ms score cp|mate ..`.

`make test` runs the tests in `tests/` (each `.in` is fed to `--test` mode and diffed against the `.out`),
`make perft` checks move generation against known perft numbers. Build with `-DCHESS_DEBUG` to verify the
incremental zobrist hash after every move. CI runs these on Linux and Windows and builds the ESP32-S3 firmware.


## How the engine works
- Board: 8x8 square array + 12 bitboards, moves packed into 16 bits, make/unmake with an undo stack.
- Incremental zobrist hash for repetition detection (threefold, 50-move rule and insufficient material are draws).
- Search: iterative deepening negamax with alpha-beta + PVS, a transposition table, killer moves, MVV-LVA move
  ordering and a capped quiescence search. Optional time limit per move.
- Eval: material, piece-square tables blended between middlegame and endgame, passed pawn bonus, and an endgame
  "mop-up" term that drives the losing king to the edge so won endgames actually get converted.
- Small opening book for the first 5 moves, a book move is only played if the search agrees it's about as good.


## ESP32-S3
The same engine runs on an ESP32-S3 (target: ESP32-S3-DevKitC-1, with octal PSRAM or none) and is played over the
USB serial console. **It builds and links but hasn't been run on hardware yet.** It is built with
[PlatformIO](https://platformio.org/) and ESP-IDF:

    pio run -e esp32s3                       # build
    pio run -e esp32s3 -t upload -t monitor  # flash and open the console (115200 baud)

The first build downloads the toolchain and takes a few minutes.

Plug into the port labelled `UART` on the devkit (the CP210x/CH340 one), PlatformIO normally finds it by itself,
otherwise add `--upload-port /dev/ttyUSB0` (`COM3` on Windows). Reset the board after flashing if the menu
doesn't show up. Exit the monitor with ctrl + c.

On boot it prints the PSRAM and transposition table size, then a menu: play, bench or perft suite. For a game it
asks which side you play, an optional FEN, search depth (default 8, max 16), time per move in ms (default 5000)
and unicode or ascii board. Moves and commands are the same as on the desktop. After each game or bench it prints
free heap, PSRAM, transposition table size and the peak stack used by the engine task, then goes back to the menu.

The engine runs in its own 48 KB task on core 1 (peak use is estimated around 37 KB at max depth) and briefly
yields about once a second.
With 8 MB octal PSRAM (N8R8, N16R8) the transposition table is 2^17 entries (~1.5 MB) in PSRAM, without PSRAM
it falls back to 2^12 entries (48 KB) in internal ram. Quad PSRAM boards (e.g. N8R2) need
`CONFIG_SPIRAM_MODE_QUAD=y` instead of `CONFIG_SPIRAM_MODE_OCT=y` in `sdkconfig.defaults`.

To use the native USB port instead of the UART one, replace `CONFIG_ESP_CONSOLE_UART_DEFAULT=y` with
`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` in `sdkconfig.defaults` (or run `pio run -e esp32s3 -t menuconfig`),
delete the generated `sdkconfig.esp32s3` and rebuild.

Any time `sdkconfig.defaults` changes (yours or after a pull), delete `sdkconfig.esp32s3` before building,
otherwise the old generated config wins and the new defaults are ignored.


## Layout
| Path | |
|---|---|
| `main.cpp` | desktop entry, parses flags |
| `src/app.cpp` | shared game loop / perft / bench used by both builds |
| `src/` | board, move gen, make/unmake, FEN, util |
| `src/ai/` | search and eval |
| `include/` | headers, `platform.h` has the desktop vs ESP32 hooks |
| `esp32/` | ESP32 entry point (serial menu) and its CMake component file |
| `platformio.ini`, `CMakeLists.txt`, `sdkconfig.defaults` | ESP32 build config |
| `tests/`, `scripts/` | test inputs/outputs, test runners, opening book scripts |
