#pragma once

#include <cstdint>
#include <string>

#include "board.h"
#include "move.h"

// Maximum possible moves in any chess position
constexpr int MAX_MOVES = 218;

// Output arrays for move generation
extern Move move_list[MAX_MOVES];
extern int move_count;
extern Move legal_move_list[MAX_MOVES];
extern int legal_move_count;

// Attack bitboards (precomputed)
extern uint64_t pawn_attacks[2][64];
extern uint64_t knight_attacks[64];
extern uint64_t king_attacks[64];


// Generate all available moves and select the 'best' one
std::string get_next_bot_move();

// Main functions to generate and filter moves
void generate_moves();
void generate_king_moves();
void generate_castling_moves();
void generate_sliding_moves();
void generate_knight_moves();
void generate_pawn_moves();
void filter_legal_moves();

// Updating board with moves
void make_move_on_board(struct Board* b, Move move);
void apply_normal_move(Board* b, int from, int to, uint8_t piece, uint8_t captured);
void apply_promotion_move(Board* b, int from, int to, uint8_t piece, int flag);
void apply_en_passant_move(Board* b, int from, int to, uint8_t piece);
void apply_castling_move(Board* b, int from, int to, uint8_t piece);
void update_castling_rights(Board* b, int from, int to, uint8_t piece, uint8_t captured);
bool king_in_check(const struct Board* b, bool side_to_move);
bool king_in_check_after_move(int from, int to);

// Setup (call at start)
void init_attack_bitboards();

// Move encoding/decoding
static inline int move_from(Move m) { return m & 0x3F; }
static inline int move_to(Move m)   { return (m >> 6) & 0x3F; }
static inline int move_flag(Move m) { return (m >> 12) & 0xF; }

// Helper: push move to list
static inline void add_move(int from, int to, int flag) {
    move_list[move_count++] = (from & 0x3F) | ((to & 0x3F) << 6) | ((flag & 0xF) << 12);
}

// Utility: get side's friendly/opponent color, index
static inline uint8_t friendly_colour() { return board.side_to_move ? BLACK : WHITE; }
static inline uint8_t opponent_colour() { return board.side_to_move ? WHITE : BLACK; }
static inline int friendly_index() { return board.side_to_move ? 1 : 0; }
static inline int opponent_index() { return board.side_to_move ? 0 : 1; }

// Utility: check if piece is friendly/opponent
static inline bool is_friendly(Piece piece) { return (piece & COLOUR_MASK) == friendly_colour(); }
static inline bool is_opponent(Piece piece) { return (piece & COLOUR_MASK) == opponent_colour(); }
