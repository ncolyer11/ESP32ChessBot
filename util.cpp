#include <cstdint>
#include <iostream>
#include <string>

#include "piece.h"

bool player1_is_human = true;
bool player2_is_human = true;

void get_player_types() {
    std::string input;
    std::cout << "Is player 1 human? (Y/n): ";
    std::getline(std::cin, input);
    player1_is_human = (input.empty() || tolower(input[0]) == 'y');
    std::cout << "Is player 2 human? (Y/n): ";
    std::getline(std::cin, input);
    player2_is_human = (input.empty() || tolower(input[0]) == 'y');
}

int file_rank_to_square_idx(int file, int rank) {
    if (rank % 2) file = 7 - file; // Flip file for snakes & ladders pattern
    int square_idx = 8 * rank + file;

    return square_idx;
}

std::pair<int, int> square_idx_to_file_rank(int square_idx) {
    int rank = square_idx / 8;
    int file = square_idx % 8;
    if (rank % 2) file = 7 - file; // Undo snakes & ladders pattern
    return {file, rank};
}

// converts say a1 to 0, h1 to 7, h2 to 8, h8 to 63
int alg_to_square_idx(const std::string& alg) {
    int file = alg[0];
    int rank = alg[1];
    if (alg.length() != 2 || file < 'a' || file > 'h' || rank < '1' || rank > '8') return -1;

    int square_idx = file_rank_to_square_idx(file - 'a', rank - '1');

    return square_idx;
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


uint8_t piece_to_bitboard_idx(uint8_t piece) {
    return (piece & TYPE_MASK) + (piece & WHITE ? 0 : 6);
}

uint8_t symbol_to_piece(char symbol) {
    uint8_t piece;
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

