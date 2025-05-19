#ifndef BOARD_H
#define BOARD_H

#include <cstdint>

struct Board {
    uint8_t squares[64];  // a1 is 0, a8 is 7, b8 is 8, h1 is 63
    uint64_t bitboards[12]; // 12 bitboards: 6 piece types x (white + black)
    
    bool side_to_move; // 0 for white, 1 for black
    uint8_t castling_rights; // 0b0000 KQkq
    uint8_t enpassant_square; // 0-63 for a1-h8, 64 for none
    uint8_t half_move_clock; // number of half moves since last pawn move or capture
    uint16_t full_moves; // starts at 1 and increments after black moves

};

extern Board board;

static const char STARTING_FEN[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

void init_board();
void print_board(bool show_params = false, bool show_bitboards = false);
void load_position_from_fen(const char* fen);

#endif
