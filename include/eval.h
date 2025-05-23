#pragma once

#include "board.h"
#include "piece.h"

struct PhaseValues {
    int pawn;
    int knight;
    int bishop;
    int rook[2];
    int queen[2];
    int bishop_pair_bonus;
};

//                                                king, pawn, knight, bishop, rook, queen
inline constexpr int simple_base_values[] = {  0, 4000, 100,  320,    330,    500,  900 };

constexpr float endgame_mat_start = simple_base_values[ROOK] * 2 
                                  + simple_base_values[BISHOP]
                                  + simple_base_values[KNIGHT];

// Piece-Square Tables sourced from: https://www.chessprogramming.org/Simplified_Evaluation_Function
constexpr int mg_pawn_pos_table[64] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
    5,  5, 10, 25, 25, 10,  5,  5,
    0,  0,  0, 20, 20,  0,  0,  0,
    5, -5,-10,  0,  0,-10, -5,  5,
    5, 10, 10,-20,-20, 10, 10,  5,
    0,  0,  0,  0,  0,  0,  0,  0
};

constexpr int eg_pawn_pos_table[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,
   30,  60,  70,  80,  80,  70,  60,  30,
   25,  50,  60,  70,  70,  60,  50,  25,
   20,  40,  50,  60,  60,  50,  40,  20,
   15,  30,  40,  50,  50,  40,  30,  15,
   10,  20,  30,  40,  40,  30,  20,  10,
    5,  10,  20,  30,  30,  20,  10,   5,
    0,   5,  10,  15,  15,  10,   5,   0
};
 

constexpr int knight_pos_table[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50,
};
constexpr int bishop_pos_table[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20,
};
constexpr int rook_pos_table[64] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    0,  0,  0,  5,  5,  0,  0,  0,
};
constexpr int queen_pos_table[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
    0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};
constexpr int mg_king_table[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20
};
constexpr int eg_king_table[64] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};

// flip for black’s perspective
static constexpr int flip_square(int square_idx) { return square_idx ^ 0b111000; }

// endgame corner‐force bonus
int force_king_to_corner_endgame_eval(int friendly_king_square,
                                      int opponent_king_square,
                                      float endgame_weight);

// full evaluation
int evaluate(Board *b);
