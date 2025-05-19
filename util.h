#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <cstdint>

extern bool player1_is_human;
extern bool player2_is_human;
extern bool player_types[2];

void get_player_types();
uint8_t piece_to_bitboard_idx(uint8_t piece);
uint8_t symbol_to_piece(char symbol);

#endif
