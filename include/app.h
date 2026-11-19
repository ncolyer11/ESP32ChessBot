#pragma once

#include <string>

#include "util.h"

enum class Mode { ASK, HVB, BVH, HVH, BVB };

struct AppConfig {
    Mode mode = Mode::ASK;
    int depth = DEFAULT_DEPTH;
    int time_ms = 0;          // 0 = no limit
    std::string fen;          // empty = start position (or prompt in test mode)
    bool test = false;        // deterministic test mode
    bool ascii_board = false; // plain ascii pieces instead of unicode
    int perft_depth = 0;      // > 0 runs perft instead of a game
    bool perft_suite = false;
    bool bench = false;
};

void app_init();                  // attack tables, tt etc, call once at startup
int app_run(const AppConfig &cfg); // runs perft/bench/game, returns the exit code
