#pragma once

#include <string>

extern const char FILE_NAMES[];
extern const char RANK_NAMES[];

// Square indices as constants
constexpr int A1 = 0;
constexpr int B1 = 1;
constexpr int C1 = 2;
constexpr int D1 = 3;
constexpr int E1 = 4;
constexpr int F1 = 5;
constexpr int G1 = 6;
constexpr int H1 = 7;

constexpr int A8 = 56;
constexpr int B8 = 57;
constexpr int C8 = 58;
constexpr int D8 = 59;
constexpr int E8 = 60;
constexpr int F8 = 61;
constexpr int G8 = 62;
constexpr int H8 = 63;

int square_idx_to_file_idx(int square_idx);
int square_idx_to_rank_idx(int square_idx);
int file_rank_to_square_idx(int file_idx, int rank_idx);
std::pair<int, int> square_idx_to_file_rank(int square_idx);
bool is_light_square(int file_idx, int rank_idx);
std::string square_name_from_coordinate(int file_idx, int rank_idx);
std::string square_name_from_idx(int square_idx);
int alg_to_square_idx(const std::string& alg);
std::string square_idx_to_alg(int square_idx);
