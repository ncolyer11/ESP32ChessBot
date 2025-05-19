#include <cstdint>
#include <iostream>
#include <string>

#include "piece.h"

bool player1_is_human = true;
bool player2_is_human = true;
bool player_types[2] = {player1_is_human, player2_is_human};


void get_player_types() {
    std::string input;
    std::cout << "Is player 1 human? (Y/n): ";
    std::getline(std::cin, input);
    player1_is_human = (input.empty() || tolower(input[0]) == 'y');
    std::cout << "Is player 2 human? (Y/n): ";
    std::getline(std::cin, input);
    player2_is_human = (input.empty() || tolower(input[0]) == 'y');
}

// Converts the 5 bits of a piece into its index (0-11) in the bitboard array
uint8_t piece_to_bitboard_idx(uint8_t piece) {
    return (piece & TYPE_MASK) + (piece & WHITE ? 0 : 6) - 1;
}

// Converts a single char/symbol, representing a piece, into its bit representation
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

