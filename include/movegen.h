#pragma once

#include <cstdint>
#include <string>

#include "board.h"
#include "move.h"

// Maximum possible moves in any chess position
constexpr int MAX_MOVES = 218;

// Output arrays for move generation
extern Move move_list[MAX_MOVES];
extern int move_count; // XXX make these unglobal as well
extern Move legal_move_list[MAX_MOVES];
extern int legal_move_count;

// Push move to list
static inline void add_move(int from, int to, int flag) {
    move_list[move_count++] = encode_move(from, to, flag);
}

// Wrapper struct for returning a copy of the legal move list and its count
struct MoveList {
    Move* moves;
    int count;
};

Move get_next_player_move(Board *b);
Move gen_and_validate_all_moves(Board *b);
bool check_end_game_conditions(Board *b, int legal_moves_count);
bool is_checkmate(int legal_moves_count, bool is_king_in_check);
bool is_draw(uint8_t half_move_clock);
bool is_stalemate(int legal_moves_count, bool is_king_in_check);

// Main functions to generate and filter moves
void generate_moves(Board *b);
MoveList generate_moves_arr(Board *b);
void generate_king_moves(Board *b);
void generate_castling_moves(Board *b);
void generate_sliding_moves(Board *b);
void generate_knight_moves(Board *b);
void generate_pawn_moves(Board *b);
void filter_legal_moves(Board *b);

// Setup (call at start)
void init_attack_bitboards();
