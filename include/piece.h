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
const char BB_PIECE_SYMBOLS[12] = {
    'K', 'P', 'N', 'B', 'R', 'Q', // White pieces (indices 0-5)
    'k', 'p', 'n', 'b', 'r', 'q'  // Black pieces (indices 6-11)
};

// Unicode chess pieces (each ends with a space for consistent width)
constexpr const char *WHITE_KING   = "♔ ";
constexpr const char *WHITE_PAWN   = "♙ ";
constexpr const char *WHITE_KNIGHT = "♘ ";
constexpr const char *WHITE_BISHOP = "♗ ";
constexpr const char *WHITE_ROOK   = "♖ ";
constexpr const char *WHITE_QUEEN  = "♕ ";
constexpr const char *BLACK_KING   = "♚ ";
constexpr const char *BLACK_PAWN   = "♟ ";
constexpr const char *BLACK_KNIGHT = "♞ ";
constexpr const char *BLACK_BISHOP = "♝ ";
constexpr const char *BLACK_ROOK   = "♜ ";
constexpr const char *BLACK_QUEEN  = "♛ ";

// Map piece encoding to sprite
inline constexpr const char *get_sprite(Piece piece) {
    switch (piece & TYPE_MASK) {
        case PAWN:   return (piece & WHITE) ? WHITE_PAWN   : BLACK_PAWN;
        case KNIGHT: return (piece & WHITE) ? WHITE_KNIGHT : BLACK_KNIGHT;
        case BISHOP: return (piece & WHITE) ? WHITE_BISHOP : BLACK_BISHOP;
        case ROOK:   return (piece & WHITE) ? WHITE_ROOK   : BLACK_ROOK;
        case QUEEN:  return (piece & WHITE) ? WHITE_QUEEN  : BLACK_QUEEN;
        case KING:   return (piece & WHITE) ? WHITE_KING   : BLACK_KING;
        case NONE:   return "  "; // Two spaces for empty square
        default:     return "? ";
    }
}

inline constexpr const char *get_sprite_from_idx(int idx) {
    switch (idx) {
        case 0: return WHITE_KING;
        case 1: return WHITE_PAWN;
        case 2: return WHITE_KNIGHT;
        case 3: return WHITE_BISHOP;
        case 4: return WHITE_ROOK;
        case 5: return WHITE_QUEEN;
        case 6: return BLACK_KING;
        case 7: return BLACK_PAWN;
        case 8: return BLACK_KNIGHT;
        case 9: return BLACK_BISHOP;
        case 10: return BLACK_ROOK;
        case 11: return BLACK_QUEEN;
        default: return "??";
    }
}