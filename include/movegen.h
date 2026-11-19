#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "board.h"
#include "move.h"

// Push move to list
static inline void add_move(MoveList *move_list, int from, int to, int flag) {
    move_list->moves[move_list->count++] = encode_move(from, to, flag);
}

Move get_next_player_move(Board *b);
Move gen_and_validate_all_moves(Board *b);
bool is_valid_move(Move *move, MoveList legal_move_list);
bool check_end_game_conditions(Board *b, int legal_moves_count);
bool is_checkmate(int legal_moves_count, bool is_king_in_check);
bool is_draw(uint8_t half_move_clock);
bool is_stalemate(int legal_moves_count, bool is_king_in_check);
bool is_repetition(const std::unordered_map<Hash, int> &board_position_counts, Hash hash);

// Main functions to generate and filter moves
void generate_moves(Board *b, MoveList *legal_move_list, bool only_captures = false);
void generate_king_moves(Board *b, MoveList *move_list, bool only_captures = false);
void generate_castling_moves(Board *b, MoveList *move_list, bool only_captures = false);
void generate_sliding_moves(Board *b, MoveList *move_list, bool only_captures = false);
void generate_knight_moves(Board *b, MoveList *move_list, bool only_captures = false);
void generate_pawn_moves(Board *b, MoveList *move_list, bool only_captures = false);
void filter_legal_moves(Board *b, const MoveList &move_list, MoveList *legal_move_list);
uint64_t perft(Board *b, int depth);

// Setup (call at start)
void init_attack_bitboards();


static constexpr Move opening_moves_encoded[5][2][10] = {
    // First move options
    {
        {0x770C, 0x76CB, 0x0546, 0x768A, 0x050C, 0x058E, 0x0449, 0x04CB, 0x774D, 0x0481 },
        //  "e4",   "d4",   "Nf3",   "c4",   "e3",   "g3",   "b3",   "d3",   "f4",   "Nc3"
        // WHITE

        {0x7934, 0x78B2, 0x78F3, 0x0B34, 0x0AB2, 0x0AF3, 0x0BB6, 0x0A71, 0x0B7E, 0x0AB9 },
        //  "e5",   "c5",   "d5",   "e6",   "c6",   "d6",   "g6",   "b6",   "Nf6",   "Nc6"
        // BLACK
    },
    // Second move options
    {
        {0x0546, 0x0685, 0x0481, 0x76CB, 0x774D, 0x04CB, 0x09C3, 0x0543, 0x048A, 0x054D },
        //  "Nf3",   "Bc4",   "Nc3",   "d4",   "f4",   "d3",   "Qh5",   "Qf3",   "c3",   "f3"
        // WHITE

        {0x0AB9, 0x0AF3, 0x0B7E, 0x08BD, 0x0B7B, 0x78F3, 0x0B75, 0x7975, 0x0AFD, 0x0AB2 },
        //  "Nc6",   "d6",   "Nf6",   "Bc5",   "Qf6",   "d5",   "f6",   "f5",   "Bd6",   "c6"
        // BLACK
    },
    // Third move options
    {
        {0x0685, 0x0845, 0x76CB, 0x0481, 0x048A, 0x04CB, 0x0305, 0x058E, 0x0408, 0x0449 },
        //  "Bc4",   "Bb5",   "d4",   "Nc3",   "c3",   "d3",   "Be2",   "g3",   "a3",   "b3"
        // WHITE

        {0x0B7E, 0x08BD, 0x0BF7, 0x0AF3, 0x06EA, 0x0D3D, 0x7975, 0x0B75, 0x0A30, 0x0B7B },
        //  "Nf6",   "Bc5",   "h6",   "d6",   "Nd4",   "Be7",   "f5",   "f6",   "a6",   "Qf6"
        // BLACK
    },
    // Fourth move options
    {
        {0x0995, 0x04CB, 0x0481, 0x2184, 0x76CB, 0x048A, 0x0D5A, 0x0303, 0x0915, 0x0408 },
        //  "Ng5",   "d3",   "Nc3",   "O-O",   "d4",   "c3",   "Bxf7+",   "Qe2",   "Nxe5",   "a3"
        // WHITE

        {0x78F3, 0x08BD, 0x072D, 0x0D3B, 0x0BF7, 0x0AF3, 0x0D3D, 0x0FBF, 0x082A, 0x06EA },
        //  "d5",   "Bc5",   "Nxe4",   "Qe7",   "h6",   "d6",   "Be7",   "Rg8",   "Na5",   "Nd4"
        // BLACK
    },
    // Fifth move options
    {
        {0x08DC, 0x08DA, 0x0D66, 0x045A, 0x085A, 0x04DA, 0x0543, 0x04CB, 0x76CB, 0x09C3 },
        //  "exd5",   "Bxd5",   "Nxf7",   "Bb3",   "Bb5",   "Bd3",   "Qf3",   "d3",   "d4",   "Qh5"
        // WHITE

        {0x08ED, 0x082A, 0x06EA, 0x7871, 0x0D2A, 0x066A, 0x0BF7, 0x07BA, 0x08BD, 0x07AD },
        //  "Nxd5",   "Na5",   "Nd4",   "b5",   "Ne7",   "Nb4",   "h6",   "Bg4",   "Bc5",   "Ng4"
        // BLACK
    },
};