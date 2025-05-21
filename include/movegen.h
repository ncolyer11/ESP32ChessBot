#pragma once

#include <cstdint>
#include <string>

#include "board.h"
#include "move.h"

// Maximum possible moves in any chess position
constexpr int MAX_MOVES = 218;

// Output arrays for move generation
extern Move move_list[MAX_MOVES];
extern int move_count;
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

Move get_next_player_move();
Move gen_and_validate_all_moves();
bool check_end_game_conditions(Board *b, int legal_moves_count);
bool is_checkmate(int legal_moves_count, bool is_king_in_check);
bool is_draw(uint8_t half_move_clock);
bool is_stalemate(int legal_moves_count, bool is_king_in_check);

// Main functions to generate and filter moves
void generate_moves();
MoveList generate_moves_arr();
void generate_king_moves();
void generate_castling_moves();
void generate_sliding_moves();
void generate_knight_moves();
void generate_pawn_moves();
void filter_legal_moves();

// Setup (call at start)
void init_attack_bitboards();
