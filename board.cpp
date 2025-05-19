#include <iostream>
#include <bitset>

#include "board.h"
#include "boardRepresentation.h"
#include "piece.h"
#include "fen.h"
#include "util.h"


Board board;

void parse_fen_flags(const char* fen, char *fen_board);

void init_board() {
    load_position_from_fen(STARTING_FEN);
    std::cout << "Board initialised." << std::endl;
}

// Default args: show_params = false, show_bitboards = false
void print_board(bool show_params, bool show_bitboards) {
    // Minimal placeholder implementation
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << rank + 1 << " ";
        for (int file = 0; file < 8; ++file) {
            int square_idx = file_rank_to_square_idx(file, rank);
            uint8_t piece = board.squares[square_idx];
            char symbol = PIECE_SYMBOLS[piece & TYPE_MASK];
            if (piece & BLACK) symbol = tolower(symbol);
            std::cout << symbol << ' ';
        }
        std::cout << std::endl;
    }
    std::cout << "  a b c d e f g h" << std::endl;

    // Show additional parameters if requested after printin
    if (show_params) {
        std::cout << std::endl;
        print_params();
    }

    // Show bitboards if requested
    if (show_bitboards) {
        std::cout << std::endl;
        print_bitboards();
    }
}
void print_params() {
    // Side to move
    std::cout << "Params: " << (board.side_to_move == 0 ? "w" : "b") << ' ';

    // Castling rights
    if (board.castling_rights & 0b1000) std::cout << 'K';
    if (board.castling_rights & 0b0100) std::cout << 'Q';
    if (board.castling_rights & 0b0010) std::cout << 'k';
    if (board.castling_rights & 0b0001) std::cout << 'q';
    if (board.castling_rights == 0) std::cout << '-';
    std::cout << ' ';

    // En passant square
    if (board.enpassant_square < 64) {
        // Print as algebraic notation (e.g., "e3") if desired
        char file = 'a' + (board.enpassant_square % 8);
        char rank = '1' + (board.enpassant_square / 8);
        std::cout << file << rank;
    } else {
        std::cout << '-';
    }
    std::cout << ' ';

    // Halfmove clock and fullmove number
    std::cout << static_cast<int>(board.half_move_clock) << ' ' << board.full_moves << std::endl;
}

void print_bitboards() {
    const char* PIECE_NAMES[12] = {
        "wk", "wp", "wn", "wb", "wr", "wq",
        "bk", "bp", "bn", "bb", "br", "bq"
    };

    std::cout << "Bitboards:" << std::endl;
    for (int i = 0; i < 12; ++i) {
        std::cout << PIECE_NAMES[i] << ": " << std::bitset<64>(board.bitboards[i]) << std::endl;
    }
}
