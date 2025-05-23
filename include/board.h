#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "piece.h"
#include "util.h"

struct UndoState {
    bool side_to_move; // 0 for white, 1 for black
    uint8_t castling_rights;
    uint8_t enpassant_square;
    uint8_t half_move_clock;
    uint16_t full_moves;
    Piece captured_piece; // The piece captured by the move being undone
};

struct Board {
    Piece squares[64];  // a1 is 0, a8 is 7, b8 is 8, h1 is 63
    U64 bitboards[12]; // 12 bitboards: 6 piece types x (white + black)
    
    bool side_to_move; // 0 for white, 1 for black
    uint8_t castling_rights; // 0b0000 KQkq
    uint8_t enpassant_square; // 0-63 for a1-h8, 64 for none
    uint8_t half_move_clock; // number of half moves since last pawn move or capture
    uint16_t full_moves; // starts at 1 and increments after black moves

    std::vector<UndoState> undo_state_stack;
};

void init_board(Board *b);
void print_board(Board *b, bool show_params = false, bool show_bitboards = false);
void print_board_from_bbs(Board *b, bool show_params = false);
void print_params(Board *b);
void print_bitboards(Board *b);

size_t hash_board_state(const Board* b);