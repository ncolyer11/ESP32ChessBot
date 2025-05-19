#pragma once

#include <cstdint>

// Move encoding: bits 0-5: from, bits 6-11: to, bits 12-15: flag
typedef uint8_t Piece;

// Piece type codes (3 bits)
constexpr Piece NONE   = 0;
constexpr Piece KING   = 1;
constexpr Piece PAWN   = 2;
constexpr Piece KNIGHT = 3;
constexpr Piece BISHOP = 4;
constexpr Piece ROOK   = 5;
constexpr Piece QUEEN  = 6;

// Colour codes (bit flags)
constexpr uint8_t WHITE = 8;   // 1 << 3
constexpr uint8_t BLACK = 16;  // 1 << 4

// Masking helpers
constexpr uint8_t TYPE_MASK = 0b00111;   // lowest 3 bits
constexpr uint8_t COLOUR_MASK = 0b11000; // bits 4 and 5

// Piece symbols for display
constexpr char PIECE_SYMBOLS[] = {'.', 'K', 'P', 'N', 'B', 'R', 'Q'};
