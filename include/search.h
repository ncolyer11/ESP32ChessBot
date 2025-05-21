#pragma once

#include "board.h"
#include "move.h"
#include "movegen.h"

Move search(const Move* legal_move_list, int legal_move_count);
Move get_next_bot_move();
