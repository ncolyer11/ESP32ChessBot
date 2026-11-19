# ESP32ChessBot
A ~1-1.5k elo chess bot written in C++ designed for speed but mainly memory efficiency so it can run on an ESP32 with only ~200kB of ram.


## Usage
Build with `make` (Linux, macOS, WSL or Windows with MinGW), the binary is `bin/chesp32` (`bin/chesp32.exe` on Windows).

    bin/chesp32 [--mode hvb|bvh|hvh|bvb] [--depth N] [--time MS] [--fen "<fen>"]
    bin/chesp32 --perft N [--fen "<fen>"]
    bin/chesp32 --bench [--depth N]

By default you are asked whether to play white, black, two players or bots only. The bot searches to `--depth`
(default 6) and optionally stops after `--time` milliseconds per move.

In game, type moves like `e2e4` (`e7e8q` to promote, queen by default) or a command: `u` undo to your last turn,
`z` undo 2 plies, `f` print FEN, `r` resign, `q` quit, `h` help.

`make test` runs the tests in `tests/` (each `.in` is fed to `--test` mode and diffed against the `.out`),
`make perft` checks move generation against known perft numbers. Build with `-DCHESS_DEBUG` to verify the
incremental zobrist hash after every move.
