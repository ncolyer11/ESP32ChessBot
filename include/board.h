#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "piece.h"
#include "util.h"


using Hash = uint64_t;
// Zobrist keys, generated at compile time from a fixed-seed splitmix64 (lives in flash on ESP32)
struct ZobristTables {
    Hash piece[12][64];
    Hash side;
    Hash castling[16];
    Hash ep_file[8];

    constexpr ZobristTables() : piece(), side(0), castling(), ep_file() {
        uint64_t state = 0x9E3779B97F4A7C15ULL;
        auto next = [&state]() constexpr {
            uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
            return z ^ (z >> 31);
        };
        for (int p = 0; p < 12; ++p)
            for (int s = 0; s < 64; ++s) piece[p][s] = next();
        side = next();
        for (int i = 0; i < 16; ++i) castling[i] = next();
        for (int i = 0; i < 8; ++i) ep_file[i] = next();
    }
};
inline constexpr ZobristTables ZOBRIST{};

struct UndoState {
    bool side_to_move; // 0 for white, 1 for black
    uint8_t castling_rights;
    uint8_t enpassant_square;
    uint8_t half_move_clock;
    uint16_t full_moves;
    Piece captured_piece; // The piece captured by the move being undone
    Hash zobrist;         // zobrist key before the move
};

struct Board {
    Piece squares[64];  // a1 is 0, a8 is 7, b8 is 8, h1 is 63
    U64 bitboards[12]; // 12 bitboards: 6 piece types x (white + black)
    
    bool side_to_move; // 0 for white, 1 for black
    uint8_t castling_rights; // 0b0000 KQkq
    uint8_t enpassant_square; // 0-63 for a1-h8, 64 for none
    uint8_t half_move_clock; // number of half moves since last pawn move or capture
    uint16_t full_moves; // starts at 1 and increments after black moves

    Hash zobrist; // incrementally maintained in make/unmake (pieces, side, castling, ep file)
    std::vector<Hash> hash_history; // zobrist of each position BEFORE each move made

    std::vector<UndoState> undo_state_stack;
};


void init_board(Board *b, const std::string &fen = "");
void print_board(Board *b, bool show_params = false, bool show_bitboards = false);
void print_board_from_bbs(Board *b, bool show_params = false);
void print_params(Board *b);
void print_bitboards(Board *b);

Hash compute_zobrist(const Board *b);       // full recompute (init + debug verification)
int repetition_count(const Board *b);       // earlier occurrences of the current position
bool is_insufficient_material(const Board *b);
