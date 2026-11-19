#include <bitset>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>

#include "piece.h"
#include "util.h"

bool MODE = false;
bool ASCII_BOARD = false;
int bot_depth = DEFAULT_DEPTH;
int bot_time_ms = 0;
bool player1_is_human = true;
bool player2_is_human = true;
bool player_types[2] = {player1_is_human, player2_is_human};


int orthogonal_distance[64][64];
int king_distance[64][64];
int centre_manhattan_distance[64];

void init_distance_tables() {
    for (int square_a = 0; square_a < 64; square_a++) {
        int file_a = square_a % 8;
        int rank_a = square_a / 8;
        int file_dist_centre = std::max(3 - file_a, file_a - 4);
        int rank_dist_centre = std::max(3 - rank_a, rank_a - 4);
        centre_manhattan_distance[square_a] = file_dist_centre + rank_dist_centre;

        for (int square_b = 0; square_b < 64; square_b++) {
            int file_b = square_b % 8;
            int rank_b = square_b / 8;
            int file_dist = std::abs(file_a - file_b);
            int rank_dist = std::abs(rank_a - rank_b);
            orthogonal_distance[square_a][square_b] = file_dist + rank_dist;
            king_distance[square_a][square_b] = std::max(file_dist, rank_dist);
        }
    }
}

void get_player_types() {
    std::string input;
    std::cout << "Is white human? (Y/n): ";
    std::getline(std::cin, input);
    player1_is_human = (input.empty() || tolower(input[0]) == 'y');
    player_types[0] = player1_is_human;
    std::cout << "Is black human? (Y/n): ";
    std::getline(std::cin, input);
    player2_is_human = (input.empty() || tolower(input[0]) == 'y');
    player_types[1] = player2_is_human;
}

// Converts the 5 bits of a piece into its index (0-11) in the bitboard array
int piece_to_bb_idx(Piece piece) {
    return (piece & TYPE_MASK) + (piece & WHITE ? 0 : 6) - 1;
}

// Converts a single char/symbol, representing a piece, into its bit representation
Piece symbol_to_piece(char symbol) {
    Piece piece;
    switch (tolower(symbol)) {
        case 'k': piece = KING; break;
        case 'p': piece = PAWN; break;
        case 'n': piece = KNIGHT; break;
        case 'b': piece = BISHOP; break;
        case 'r': piece = ROOK; break;
        case 'q': piece = QUEEN; break;
        default: piece = NONE; break;
    }

    if (piece != NONE) piece |= (isupper(symbol) ? WHITE : BLACK);
    return piece;
}

//
/**
 * @brief Finds the index of the least significant set bit in a 64-bit bitboard.
 *
 * Index (0-based) of the least significant bit that is set to 1 in the given 64-bit integer.
 *
 * @param bitboard The 64-bit unsigned integer to scan.
 * @return int The index (0-63) of the least significant set bit in bb.
 *         The result is undefined if bb is zero.
 */
int bit_scan_forward(U64 bitboard) {
    return __builtin_ctzll(bitboard);
}

/**
 * @brief Counts the number of set bits (1s) in a 64-bit integer.
 */
int popcount(uint64_t x) {
    return __builtin_popcountll(x);
}

// strip UTF-8 BOM (0xEF,0xBB,0xBF) and trailing '\r'
void normalise_line(std::string &s) {
    if (s.size() >= 3
     && (unsigned char)s[0] == 0xEF
     && (unsigned char)s[1] == 0xBB
     && (unsigned char)s[2] == 0xBF)
        s.erase(0, 3);
    if (!s.empty() && s.back() == '\r')
        s.pop_back();
}
