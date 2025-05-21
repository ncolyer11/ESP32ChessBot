#pragma once

#include <iostream>
#include <cctype>
#include <cstdint>

#include "piece.h"

typedef uint64_t U64;

extern bool MODE; // false = play, true = testing
extern bool player1_is_human;
extern bool player2_is_human;
extern bool player_types[2];

constexpr const char *RESET    = "\033[0m";
constexpr const char *DIM      = "\033[2m";
constexpr const char *BRIGHT   = "\033[1m";
constexpr const char *WHITE_BG = "\033[47m";
constexpr const char *GREEN_BG = "\033[42m";
constexpr const char *BLUE_BG = "\033[104m";
constexpr const char *WHITE_FG = "\033[97m";
constexpr const char *BLACK_FG = "\033[34m"; // blue for black pieces

constexpr const int ENPASSANT_FLAG = 1;
constexpr const int CASTLING_FLAG = 2;
constexpr const int PROMOTION_FLAG = 3;
constexpr const int PROMOTION_QUEEN_FLAG = 3;
constexpr const int PROMOTION_KNIGHT_FLAG = 4;
constexpr const int PROMOTION_ROOK_FLAG = 5;
constexpr const int PROMOTION_BISHOP_FLAG = 6;
constexpr const int DOUBLE_PAWN_FLAG = 7;

void get_player_types();
int piece_to_bitboard_idx(Piece piece);
Piece symbol_to_piece(char symbol);
int bit_scan_forward(U64 bb);

// strip UTF-8 BOM (0xEF,0xBB,0xBF) and trailing '\r'
void normalise_line(std::string &s);
