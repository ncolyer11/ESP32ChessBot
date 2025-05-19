#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <cstdint>

#include "piece.h"

extern bool player1_is_human;
extern bool player2_is_human;
extern bool player_types[2];

void get_player_types();
uint8_t piece_to_bitboard_idx(Piece piece);
Piece symbol_to_piece(char symbol);

#endif
