#pragma once

#include <string>

#include "piece.h"
#include "util.h"

// Move encoding: bits 0-5: from, bits 6-11: to, bits 12-15: flag
typedef uint16_t Move;

// Maximum possible moves in any chess position
constexpr int MAX_MOVES = 218;

struct MoveList {
    Move moves[MAX_MOVES];
    int count;
};

extern std::vector<Move> move_history; // For undos and debugging

constexpr const int ENPASSANT_FLAG = 1;
constexpr const int CASTLING_FLAG = 2;
constexpr const int PROMOTION_FLAG = 3;
constexpr const int PROMOTION_QUEEN_FLAG = 3;
constexpr const int PROMOTION_KNIGHT_FLAG = 4;
constexpr const int PROMOTION_ROOK_FLAG = 5;
constexpr const int PROMOTION_BISHOP_FLAG = 6;
constexpr const int DOUBLE_PAWN_FLAG = 7;

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
static inline uint8_t friendly_colour(Board *b) { return b->side_to_move ? BLACK : WHITE; }
static inline uint8_t opponent_colour(Board *b) { return b->side_to_move ? WHITE : BLACK; }

// Utility: check if piece is friendly/opponent
static inline bool is_friendly(Board *b, Piece piece) { 
    return (piece & COLOUR_MASK) == friendly_colour(b);
}
static inline bool is_opponent(Board *b, Piece piece) { 
    return (piece & COLOUR_MASK) == opponent_colour(b);
}


// Updating board with moves
bool make_move_on_board(Board *b, Move move, bool in_search = false);
void unmake_move_on_board(Board *b, Move move, bool in_search = false);
bool king_in_check(Board *b);
bool king_in_check_after_move(Board *b, int from, int to);

std::string move_to_move_str(Move move);
Move move_str_to_move(std::string move_str);
bool validate_move_encoding(Move move);

void quicksort_moves(Move *moves, int *scores, int low, int high);
