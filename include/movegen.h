#pragma once

#include <cstdint>
#include <string>

#include "board.h"
#include "move.h"

// Push move to list
static inline void add_move(MoveList *move_list, int from, int to, int flag) {
    move_list->moves[move_list->count++] = encode_move(from, to, flag);
}

Move get_next_player_move(Board *b);
Move gen_and_validate_all_moves(Board *b);
bool is_valid_move(Move *move, MoveList legal_move_list);
bool check_end_game_conditions(Board *b, int legal_moves_count);
bool is_checkmate(int legal_moves_count, bool is_king_in_check);
bool is_draw(uint8_t half_move_clock);
bool is_stalemate(int legal_moves_count, bool is_king_in_check);

// Main functions to generate and filter moves
void generate_moves(Board *b, MoveList *legal_move_list);
void filter_legal_moves(Board *b, MoveList move_list, MoveList *legal_move_list);
void generate_king_moves(Board *b, MoveList *move_list);
void generate_castling_moves(Board *b, MoveList *move_list);
void generate_sliding_moves(Board *b, MoveList *move_list);
void generate_knight_moves(Board *b, MoveList *move_list);
void generate_pawn_moves(Board *b, MoveList *move_list);

// Setup (call at start)
void init_attack_bitboards();
