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

extern int orthogonal_distance[64][64];
extern int king_distance[64][64];
extern int centre_manhattan_distance[64];

constexpr int DEPTH = 6; // depth for the bot to search

constexpr const char *RESET      = "\033[0m";
constexpr const char *RESET_LINE = "\033[0K";
constexpr const char *DIM        = "\033[2m";
constexpr const char *BRIGHT     = "\033[1m";
constexpr const char *WHITE_BG   = "\033[47m";
constexpr const char *GREEN_BG   = "\033[42m";
constexpr const char *BLUE_BG    = "\033[104m";
constexpr const char *WHITE_FG   = "\033[97m";
constexpr const char *BLACK_FG   = "\033[34m";

void init_distance_tables();
void get_player_types();
int piece_to_bb_idx(Piece piece);
Piece symbol_to_piece(char symbol);
int bit_scan_forward(U64 bb);
int popcount(uint64_t x);
void test_popcount(U64 x);

void check_for_test_mode();
// strip UTF-8 BOM (0xEF,0xBB,0xBF) and trailing '\r'
void normalise_line(std::string &s);