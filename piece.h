#ifndef PIECE_H
#define PIECE_H

#include <cstdint>

// Piece type codes (3 bits)
constexpr uint8_t NONE   = 0;
constexpr uint8_t KING   = 1;
constexpr uint8_t PAWN   = 2;
constexpr uint8_t KNIGHT = 3;
constexpr uint8_t BISHOP = 4;
constexpr uint8_t ROOK   = 5;
constexpr uint8_t QUEEN  = 6;

// Colour codes (bit flags)
constexpr uint8_t WHITE = 8;   // 1 << 3
constexpr uint8_t BLACK = 16;  // 1 << 4

// Masking helpers
constexpr uint8_t TYPE_MASK = 0b00111;   // lowest 3 bits
constexpr uint8_t COLOUR_MASK = 0b11000; // bits 4 and 5
constexpr uint8_t WHITE_MASK = 0b01000;  // bit 4
constexpr uint8_t BLACK_MASK = 0b10000; // bit 5


// Piece symbols for display
constexpr char PIECE_SYMBOLS[] = {
    '.', 'K', 'P', 'N', 'B', 'R', 'Q'
};

#endif
