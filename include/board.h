#pragma once

#include <cstdint>

#include "piece.h"
#include "util.h"

struct Board {
    Piece squares[64];  // a1 is 0, a8 is 7, b8 is 8, h1 is 63
    U64 bitboards[12]; // 12 bitboards: 6 piece types x (white + black)
    
    bool side_to_move; // 0 for white, 1 for black
    uint8_t castling_rights; // 0b0000 KQkq
    uint8_t enpassant_square; // 0-63 for a1-h8, 64 for none
    uint8_t half_move_clock; // number of half moves since last pawn move or capture
    uint16_t full_moves; // starts at 1 and increments after black moves

    struct {
        Piece    captured_piece;
        uint8_t  castling_rights;
        uint8_t      enpassant_square;
        uint8_t      half_move_clock;
        uint16_t      full_moves;
    } undo_state;
};

extern Board board;

void init_board();
void print_board(bool show_params = false, bool show_bitboards = false);
void print_params();
void print_bitboards();
