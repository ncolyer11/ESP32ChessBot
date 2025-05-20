#include <iostream>
#include <string>

#include "boardRepresentation.h"

const char FILE_NAMES[] = "abcdefgh";
const char RANK_NAMES[] = "12345678";

int square_idx_to_file_idx(int square_idx) {
    return square_idx & 0b000111;
}

int square_idx_to_rank_idx(int square_idx) {
    return square_idx >> 3;
}

int file_rank_to_square_idx(int file_idx, int rank_idx) {
    return rank_idx * 8 + file_idx;
}

std::pair<int, int> square_idx_to_file_rank(int square_idx) {
    return {square_idx_to_file_idx(square_idx), square_idx_to_rank_idx(square_idx)};
}

bool is_light_square(int file_idx, int rank_idx) {
    return ((file_idx + rank_idx) % 2 == 1);
}

std::string square_name_from_coordinate(int file_idx, int rank_idx) {
    std::string out;
    out += FILE_NAMES[file_idx];
    out += RANK_NAMES[rank_idx];
    return out;
}

std::string square_name_from_idx(int square_idx) {
    return square_name_from_coordinate(square_idx_to_file_idx(square_idx),
                                       square_idx_to_file_idx(square_idx));
}

// converts say a1 to 0, h1 to 7, h2 to 8, h8 to 63
int alg_to_square_idx(const std::string& alg) {
    if (alg.length() != 2) return -1;
    int file = alg[0];
    int rank = alg[1];
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') return -1;

    return file_rank_to_square_idx(file - 'a', rank - '1');
}

// converts say 0 to a1, 7 to h1, 8 to a2, 63 to h8. alg short for algebraic notation
std::string square_idx_to_alg(int square_idx) {
    if (square_idx < 0 || square_idx >= 64) return "INVALID";

    std::pair<int, int> res = square_idx_to_file_rank(square_idx);
    int file = res.first;
    int rank = res.second;

    std::string alg;
    alg += 'a' + file;
    alg += '1' + rank;

    return alg;
}

