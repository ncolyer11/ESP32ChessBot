#pragma once

#include <string_view>

#include "board.h"
#include "move.h"

Move get_next_bot_move(Board *b, int depth);
bool get_check(Board *b, Move move, int id);

