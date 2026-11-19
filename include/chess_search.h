#pragma once

#include <string_view>

#include "board.h"
#include "move.h"

// depth is the max search depth; time_ms > 0 also limits thinking time (0 = no limit)
Move get_next_bot_move(Board *b, int depth, int time_ms = 0);
bool get_check(Board *b, Move move, int id);

// infinities
constexpr int pos_infinity = 10000000; 
constexpr int neg_infinity = -pos_infinity;

constexpr int MATE_BASE_SCORE = pos_infinity;
constexpr int SEARCH_ALPHA_BETA_INF = MATE_BASE_SCORE + 1000;

