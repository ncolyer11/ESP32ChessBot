#include <bitset>
#include <iostream>

#include "board.h"
#include "boardRepresentation.h"
#include "fen.h"
#include "movegen.h"
#include "piece.h"
#include "util.h"

void init_board(Board *b) {
    std::string fen = starting_fen;
    if (MODE) fen = choose_fen();
    load_position_from_fen(b, fen);
    std::cout << "Board initialised.\n" << std::endl;
}

// Default args: show_params = false, show_bitboards = false
void print_board(Board *b, bool show_params, bool show_bitboards) {
    for (int rank = 7; rank >= 0; --rank) {
        // Rank label
        std::cout << (rank + 1) << " ";
        for (int file = 0; file < 8; ++file) {
            Piece piece = b->squares[file_rank_to_square_idx(file, rank)];
            
            if (MODE) {
                char symbol = PIECE_SYMBOLS[piece & TYPE_MASK];
                symbol = (piece & BLACK) ? tolower(symbol) : toupper(symbol);
                std::cout << symbol << " ";
                continue;
            }
            
            // Square color: light (white) or dark (green/blue)
            const char *bg = is_light_square(file, rank) ? WHITE_BG : GREEN_BG;
            std::cout << bg << get_sprite(piece) << RESET;
        }
        std::cout << std::endl;
    }

    // File labels
    std::cout << "  ";
    for (char c = 'a'; c <= 'h'; ++c) {
        std::cout << c << ' ';
    }
    std::cout << std::endl;

    // Show additional parameters if requested
    if (show_params) {
        std::cout << std::endl;
        print_params(b);
    }

    // Show bitboards if requested
    if (show_bitboards) {
        print_bitboards(b);
        std::cout << std::endl;
    }
}

void print_board_from_bbs(Board *b, bool show_params) {
    for (int rank = 7; rank >= 0; --rank) {
        // Rank label
        std::cout << (rank + 1) << " ";
        for (int file = 0; file < 8; ++file) {
            int square_idx = file_rank_to_square_idx(file, rank);
            const char *symbol = "  "; // Default to empty square

            // Check each of the 12 bitboards for a piece on this square
            int found = 0;
            for (int bb_idx = 0; bb_idx < 12; ++bb_idx) {
                // Check if the bit for the current square_idx is set in the current bitboard
                if ((b->bitboards[bb_idx] >> square_idx) & 1ULL) {
                    found++;
                    if (found == 1) {
                        symbol = get_sprite_from_idx(bb_idx);
                    } else {
                        static char buf[3];
                        snprintf(buf, sizeof(buf), "%02d", found);
                        symbol = buf;
                    }
                }
            }
            const char *bg = is_light_square(file, rank) ? WHITE_BG : GREEN_BG;
            std::cout << bg << symbol << RESET;
        }
        std::cout << std::endl;
    }

    // File labels
    std::cout << "  ";
    for (char c = 'a'; c <= 'h'; ++c) {
        std::cout << c << ' ';
    }
    std::cout << std::endl;

    // Show additional parameters if requested
    if (show_params) {
        std::cout << std::endl;
        print_params(b);
    }
}

void print_params(Board *b) {
    // Side to move
    std::cout << "Params: " << (b->side_to_move == 0 ? "w" : "b") << ' ';

    // Castling rights
    if (b->castling_rights & 0b0001) std::cout << 'K';
    if (b->castling_rights & 0b0010) std::cout << 'Q';
    if (b->castling_rights & 0b0100) std::cout << 'k';
    if (b->castling_rights & 0b1000) std::cout << 'q';
    if (b->castling_rights == 0) std::cout << '-';
    std::cout << ' ';

    // En passant square
    if (b->enpassant_square < 64) {
        // Print as algebraic notation (e.g., "e3") if desired
        char file = 'a' + (b->enpassant_square % 8);
        char rank = '1' + (b->enpassant_square / 8);
        std::cout << file << rank;
    } else {
        std::cout << '-';
    }
    std::cout << ' ';

    // Halfmove clock and fullmove number
    std::cout << static_cast<int>(b->half_move_clock) << ' ' << b->full_moves << std::endl;
}

void print_bitboards(Board *b) {
    const char *PIECE_NAMES[12] = {
        "wk", "wp", "wn", "wb", "wr", "wq",
        "bk", "bp", "bn", "bb", "br", "bq"
    };

    std::cout << "Bitboards:" << std::endl;
    for (int i = 0; i < 12; ++i) {
        std::cout << PIECE_NAMES[i] << ": " << std::bitset<64>(b->bitboards[i]) << std::endl;
    }
}

void print_bitboard_history(Board b_history[BITBOARD_HISTORY_SIZE], 
                            U64 bitboard_history[BITBOARD_HISTORY_SIZE][2], int bitboard_history_index) {
    std::cout << "Bitboard History (Oldest to Newest):" << std::endl;
    // Start from the element *after* the current index.
    // This is the oldest element that is still in the buffer if the buffer is full.
    // If the buffer is not yet full, elements before the index will be 0.
    int start_index = bitboard_history_index;

    std::cout << "  White Bitboards:" << std::endl;
    for (int i = 0; i < BITBOARD_HISTORY_SIZE; ++i) {
        int current_read_index = (start_index + i) % BITBOARD_HISTORY_SIZE;
        U64 white_bb = bitboard_history[current_read_index][0];
        if (white_bb != 0) {
            std::cout << "    [" << i << "] 0b" << std::bitset<64>(white_bb) << std::endl;
        } else {
            std::cout << "    [" << i << "] (Empty/Initial)" << std::endl;
        }
    }
    std::cout << "  Black Bitboards:" << std::endl;
    for (int i = 0; i < BITBOARD_HISTORY_SIZE; ++i) {
        int current_read_index = (start_index + i) % BITBOARD_HISTORY_SIZE;
        U64 black_bb = bitboard_history[current_read_index][1];
        if (black_bb != 0) {
            std::cout << "    [" << i << "] 0b" << std::bitset<64>(black_bb) << std::endl;
        } else {
            std::cout << "    [" << i << "] (Empty/Initial)" << std::endl;
        }
    }
    std::cout << "  Board History:" << std::endl;
    for (int i = 0; i < BITBOARD_HISTORY_SIZE; ++i) {
        int current_read_index = (start_index + i) % BITBOARD_HISTORY_SIZE;
        std::cout << "    [" << i << "] " << std::endl;
        print_board(&b_history[current_read_index], true, true);
    }
    std::cout << "------------------------------------" << std::endl;
}


bool exit_if_corrupt_bits(Board *b, Board b_history[BITBOARD_HISTORY_SIZE], 
                          U64 bitboard_history[BITBOARD_HISTORY_SIZE][2], int bitboard_history_index, int id) {
    // Print out both pawn bitboards
    for (int i = 1; i <= 7; i += 6) {
        U64 bb = b->bitboards[i];
        int count = 0;
        U64 og_bb = bb;
        while (bb > 0) {
            bb &= (bb - 1); // Clear the least significant set bit
            count++;
            // Early exit: If we already have more than 8 bits, no need to count further
            if (count > 8) {
                std::cerr << "Corrupted bitboard [" << id << "]:" << std::bitset<64>(og_bb) << std::endl;
                std::cerr << "Exiting..." << std::endl;
                print_bitboard_history(b_history, bitboard_history, bitboard_history_index);
                return true;
            }
        }
    }

    return false;
}

size_t hash_board_state(const Board* b) {
    size_t seed = 0;

    for (int i = 0; i < 64; ++i) {
        seed ^= std::hash<uint8_t>{}(static_cast<uint8_t>(b->squares[i])) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    for (int i = 0; i < 12; ++i) {
        seed ^= std::hash<U64>{}(b->bitboards[i]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    seed ^= std::hash<uint8_t>{}(b->side_to_move) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<uint8_t>{}(b->castling_rights) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<uint8_t>{}(b->enpassant_square) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<uint8_t>{}(b->half_move_clock) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<uint16_t>{}(b->full_moves) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    // Hash the undo state stack
    for (const auto& state : b->undo_state_stack) {
        seed ^= std::hash<bool>{}(state.side_to_move) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<uint8_t>{}(state.castling_rights) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<uint8_t>{}(state.enpassant_square) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<uint8_t>{}(state.half_move_clock) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<uint16_t>{}(state.full_moves) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    return seed;
}