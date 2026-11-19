#include <bitset>
#include <iostream>

#include "board.h"
#include "boardRepresentation.h"
#include "fen.h"
#include "movegen.h"
#include "piece.h"
#include "util.h"

Hash compute_zobrist(const Board *b) {
    Hash h = 0;
    for (int sq = 0; sq < 64; ++sq) {
        Piece p = b->squares[sq];
        if ((p & TYPE_MASK) != NONE) h ^= ZOBRIST.piece[piece_to_bb_idx(p)][sq];
    }
    if (b->side_to_move) h ^= ZOBRIST.side;
    h ^= ZOBRIST.castling[b->castling_rights & 15];
    if (b->enpassant_square < 64) h ^= ZOBRIST.ep_file[b->enpassant_square & 7];
    return h;
}

// How many times the current position occurred earlier. Only same-side-to-move
// positions (every 2nd ply) within the last half_move_clock plies can match.
int repetition_count(const Board *b) {
    int n = (int)b->hash_history.size();
    int count = 0;
    for (int k = 2; k <= (int)b->half_move_clock && k <= n; k += 2) {
        if (b->hash_history[n - k] == b->zobrist) count++;
    }
    return count;
}

bool is_insufficient_material(const Board *b) {
    // bitboard order: K P N B R Q (white), then black
    if (b->bitboards[1] | b->bitboards[4] | b->bitboards[5] |
        b->bitboards[7] | b->bitboards[10] | b->bitboards[11]) return false;
    U64 wn = b->bitboards[2], wb = b->bitboards[3], bn = b->bitboards[8], bb = b->bitboards[9];
    int minors = popcount(wn | wb | bn | bb);
    if (minors <= 1) return true;
    // K+B v K+B with bishops on the same colour squares
    if (wn == 0 && bn == 0 && popcount(wb) == 1 && popcount(bb) == 1) {
        constexpr U64 LIGHT = 0x55AA55AA55AA55AAULL;
        return ((wb & LIGHT) != 0) == ((bb & LIGHT) != 0);
    }
    return false;
}

// fen empty: starting position (or the stdin prompt in test mode)
void init_board(Board *b, const std::string &fen_arg) {
    std::string fen = fen_arg;
    if (fen.empty()) fen = MODE ? choose_fen() : starting_fen;
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
            
            if (MODE || ASCII_BOARD) {
                char symbol = PIECE_SYMBOLS[piece & TYPE_MASK];
                symbol = (piece & BLACK) ? tolower(symbol) : toupper(symbol);
                std::cout << symbol << " ";
                continue;
            }
            
            // Square color: light (white) or dark (green/blue)
            const char *bg = is_light_square(file, rank) ? WHITE_BG : GREEN_BG;
            std::cout << bg << get_sprite(piece) << RESET;
        }
        if (!MODE && !ASCII_BOARD) std::cout << RESET_LINE;
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
                        if (ASCII_BOARD) {
                            static char letter[3] = "  ";
                            letter[0] = BB_PIECE_SYMBOLS[bb_idx];
                            symbol = letter;
                        } else {
                            symbol = get_sprite_from_idx(bb_idx);
                        }
                    } else {
                        static char buf[16];
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
