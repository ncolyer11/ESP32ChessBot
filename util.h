#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <cstdint>

extern bool player1_is_human;
extern bool player2_is_human;
extern bool player_types[2];

void get_player_types();
int file_rank_to_square_idx(int file, int rank);
std::pair<int, int> square_idx_to_file_rank(int square_idx);
int alg_to_square_idx(const std::string& alg);
std::string square_idx_to_alg(int square_idx);
uint8_t piece_to_bitboard_idx(uint8_t piece);
uint8_t symbol_to_piece(char symbol);

#endif
