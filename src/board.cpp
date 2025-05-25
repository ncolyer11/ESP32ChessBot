#include <bitset>
#include <iostream>

#include "board.h"
#include "boardRepresentation.h"
#include "fen.h"
#include "movegen.h"
#include "piece.h"
#include "util.h"

HashCountMap board_pos_counts;
HashHistory board_pos_history;

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
        std::cout << RESET_LINE << std::endl;
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

Hash hash_last_undo_states(const Board* b, Hash seed) {
    Hash num_to_hash = std::min<Hash>(5, b->undo_state_stack.size());
    Hash start_idx = b->undo_state_stack.size() - num_to_hash;

    for (Hash i = start_idx; i < b->undo_state_stack.size(); ++i) {
        const auto& state = b->undo_state_stack[i];
        seed ^= std::hash<bool>{}(state.side_to_move) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<uint8_t>{}(state.castling_rights) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<uint8_t>{}(state.enpassant_square) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<uint8_t>{}(state.half_move_clock) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<uint16_t>{}(state.full_moves) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    return seed;
}

Hash hash_board_state(const Board* b, bool squares_only) {
    Hash seed = 0;

    for (int i = 0; i < 64; ++i) {
        seed ^= std::hash<uint8_t>{}(static_cast<uint8_t>(b->squares[i])) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    if (squares_only) {
        return seed; // Return early if only squares are to be hashed
    }

    for (int i = 0; i < 12; ++i) {
        seed ^= std::hash<U64>{}(b->bitboards[i]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    seed ^= std::hash<uint8_t>{}(b->side_to_move) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<uint8_t>{}(b->castling_rights) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<uint8_t>{}(b->enpassant_square) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<uint8_t>{}(b->half_move_clock) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<uint16_t>{}(b->full_moves) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    
    // Hash only the last 5 undo states (or fewer if stack is smaller)
    seed = hash_last_undo_states(b, seed);

    return seed;
}

void add_position_hash(HashHistory& history, HashCountMap& counts, Hash hash) {
    history.push_back(hash);
    HashCountMapIt it = counts.find(hash);
    if (it != counts.end()) {
        it->second++;
    } else {
        counts[hash] = 1;
    }
}

void remove_last_position_hash(HashHistory& history, HashCountMap& counts) {
    if (!history.empty()) {
        Hash last = history.back();
        history.pop_back();

        HashCountMapIt it = counts.find(last);
        if (it != counts.end()) {
            if (--(it->second) == 0) {
                counts.erase(it);
            }
        }
    }
}
