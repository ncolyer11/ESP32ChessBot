#pragma once

#include <string>

#include "piece.h"
#include "util.h"

// Move encoding: bits 0-5: from, bits 6-11: to, bits 12-15: flag
typedef uint16_t Move;

constexpr const Move INVALID_MOVE = 0xFFFF;
constexpr const Move NO_MOVES = 0x0000;

// Attack bitboards (precomputed)
extern U64 pawn_attacks[2][64];
extern U64 knight_attacks[64];
extern U64 king_attacks[64];

// Move encoding/decoding
static inline int move_from(Move m) { return m & 0x3F; }
static inline int move_to(Move m)   { return (m >> 6) & 0x3F; }
static inline int move_flag(Move m) { return (m >> 12) & 0xF; }
static inline Move encode_move(int from, int to, int flag) {
    return static_cast<Move>(from & 0x3F) | ((to & 0x3F) << 6) | ((flag & 0xF) << 12);
}

// Utility: get side's friendly/opponent color, index
static inline uint8_t friendly_colour() { return board.side_to_move ? BLACK : WHITE; }
static inline uint8_t opponent_colour() { return board.side_to_move ? WHITE : BLACK; }
static inline int friendly_index() { return board.side_to_move ? 1 : 0; }
static inline int opponent_index() { return board.side_to_move ? 0 : 1; }

// Utility: check if piece is friendly/opponent
static inline bool is_friendly(Piece piece) { return (piece & COLOUR_MASK) == friendly_colour(); }
static inline bool is_opponent(Piece piece) { return (piece & COLOUR_MASK) == opponent_colour(); }


bool is_valid_move(Move *move);

// Updating board with moves
bool make_move_on_board(struct Board *b, Move move);
void apply_normal_move(Board *b, int from, int to, Piece piece, Piece captured);
void apply_promotion_move(Board *b, int from, int to, Piece piece, int flag);
void apply_en_passant_move(Board *b, int from, int to, Piece piece);
void apply_castling_move(Board *b, int from, int to, Piece piece);
void update_castling_rights(Board *b, int from, int to, Piece piece, Piece captured);
bool king_in_check(const struct Board *b, bool side_to_move);
bool king_in_check_after_move(int from, int to);

std::string move_to_move_str(Move move);
Move move_str_to_move(std::string move_str);
bool validate_move_encoding(Move move);
