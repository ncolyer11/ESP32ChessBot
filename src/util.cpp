#include <cstdint>
#include <iostream>
#include <string>

#include "piece.h"
#include "util.h"

bool MODE = false;
bool player1_is_human = true;
bool player2_is_human = true;
bool player_types[2] = {player1_is_human, player2_is_human};


void get_player_types() {
    std::string input;
    std::cout << "Is player 1 human? (Y/n): ";
    std::getline(std::cin, input);
    player1_is_human = (input.empty() || tolower(input[0]) == 'y');
    player_types[0] = player1_is_human;
    std::cout << "Is player 2 human? (Y/n): ";
    std::getline(std::cin, input);
    player2_is_human = (input.empty() || tolower(input[0]) == 'y');
    player_types[1] = player2_is_human;
}

// Converts the 5 bits of a piece into its index (0-11) in the bitboard array
Piece piece_to_bitboard_idx(Piece piece) {
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
 * This function uses a de Bruijn sequence to efficiently determine the position
 * (0-based index) of the least significant bit that is set to 1 in the given 64-bit integer.
 *
 * @param bitboard The 64-bit unsigned integer to scan.
 * @return int The index (0-63) of the least significant set bit in bb.
 *         The result is undefined if bb is zero.
 */
int bit_scan_forward(U64 bitboard) {
   static const int index64[64] = {
      0 , 1,48, 2,57,49,28, 3,61,58,50,42,38,29,17, 4,
      62,55,59,36,53,51,43,22,45,39,33,30,24,18,12, 5,
      63,47,56,27,60,41,37,16,54,35,52,21,44,32,23,11,
      46,26,40,15,34,20,31,10,25,14,19, 9,13, 8, 7, 6
   };
   return index64[((bitboard & -bitboard) * 0x03f79d71b4cb0a89ULL) >> 58];
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
