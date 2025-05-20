#pragma once

#include <iostream>
#include <cctype>
#include <cstdint>

#include "piece.h"

extern bool player1_is_human;
extern bool player2_is_human;
extern bool player_types[2];

typedef uint64_t U64;

constexpr const char* RESET    = "\033[0m";
constexpr const char* DIM      = "\033[2m";
constexpr const char* BRIGHT   = "\033[1m";
constexpr const char* WHITE_BG = "\033[47m";
constexpr const char* GREEN_BG = "\033[42m";
constexpr const char* BLUE_BG = "\033[104m";
constexpr const char* WHITE_FG = "\033[97m";
constexpr const char* BLACK_FG = "\033[34m"; // blue for black pieces

void get_player_types();
Piece piece_to_bitboard_idx(Piece piece);
Piece symbol_to_piece(char symbol);
int bit_scan_forward(U64 bb);
