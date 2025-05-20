#include <bitset>
#include <cstring>
#include <cctype>
#include <iostream>
#include <limits>
#include <regex>
#include <string>

#include "board.h"
#include "boardRepresentation.h"
#include "piece.h"
#include "fen.h"
#include "util.h"

void parse_and_load_fen_params(const char*rest);
void parse_and_export_fen_params(std::string fen);


/**
 * @brief Loads the board position from a FEN string.
 *
 * The FEN string should be in standard format:
 *   "piece_placements active_color castling_availability en_passant_square halfmove_clock fullmove"
 * For example: "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
 *
 * @param fen The FEN string describing the board state.
 */
void load_position_from_fen(std::string fen) {
    // Clear old Board state
    std::memset(board.squares, NONE, sizeof(board.squares));
    std::memset(board.bitboards, 0, sizeof(board.bitboards));
    std::cout << "Loading position from FEN: " << fen << std::endl;
    int file = 0, rank = 7;

    // Only take board layout (before first space)
    char fen_board[128];
    int i;
    for (i = 0; fen[i] != ' ' && fen[i] != '\0' && i < 127; ++i) fen_board[i] = fen[i];
    fen_board[i] = '\0';

    for (int idx = 0; fen_board[idx] != '\0'; idx++) {
        char symbol = fen_board[idx];
        if (symbol == '/') {
            file = 0;
            rank--;
        } else if (isdigit(symbol)) {
            file += symbol - '0';
        } else {
            Piece piece = symbol_to_piece(symbol);
            uint8_t square_idx = file_rank_to_square_idx(file, rank);
            // 64-bit mask with 1 at the square position, 1ULL is a 64-bit unsigned long long
            U64 square_mask = 1ULL << square_idx;
            board.squares[square_idx] = piece;
            board.bitboards[piece_to_bitboard_idx(piece)] |= square_mask;
            file++;
        }
    }

    // Parse the rest of the FEN parameters
    parse_and_load_fen_params(fen.c_str() + strlen(fen_board));
}

/**
 * @brief Exports the current board state as a FEN string.
 * @return The FEN string representation of the board.
 */
std::string export_position_to_fen() {
    std::string fen;
    for (int rank = 7; rank >= 0; --rank) {
        int empty_count = 0;
        for (int file = 0; file < 8; ++file) {
            Piece piece = board.squares[rank * 8 + file];
            if (piece == NONE) {
                empty_count++;
            } else {
                if (empty_count > 0) {
                    fen += std::to_string(empty_count);
                    empty_count = 0;
                }
                int piece_index = (piece & TYPE_MASK) + (piece & WHITE ? 0 : 6);
                char symbol = PIECE_SYMBOLS[piece_index];
                symbol = (piece & BLACK) ? tolower(symbol) : toupper(symbol);
                fen += symbol;
            }
        }
        if (empty_count > 0) fen += std::to_string(empty_count);
        if (rank != 0) fen += '/';
    }

    // Parse and export the FEN parameters
    parse_and_export_fen_params(fen);

    return fen;
}

std::string choose_fen() {
    std::string fen;
    std::cout << "Enter a starting position (FEN or @custom_name): ";
    std::getline(std::cin, fen);
    std::cout << "Read: " << fen << std::endl;
    if (!fen.empty() && fen[0] == '@') {
        fen = fen.substr(1, fen.length() - 1);

        if (fen.empty() || fen == "default") return starting_fen;
        if (fen == "queens") return queens_fen;
        if (fen == "stalemate") return stalemate_fen;
        if (fen == "check") return check_fen;
        if (fen == "coverage") return coverage_fen;
        if (fen == "castle") return castling_fen;
    }

    if (std::regex_match(fen, fen_regex)) return fen;
    
    std::cout << "Invalid FEN format. Using default starting position." << std::endl;
    return starting_fen;
}



// After parsing board layout, parse rest of FEN:
void parse_and_load_fen_params(const char *rest) {
    if (*rest == ' ') rest++;

    // 2. Parse side to move
    board.side_to_move = (*rest == 'w') ? 0 : 1;
    while (*rest && *rest != ' ') rest++; // skip to next field
    if (*rest == ' ') rest++;

    // 3. Parse castling rights
    board.castling_rights = 0;
    if (*rest == '-') {
        rest++;
    } else {
        for (; *rest && *rest != ' '; rest++) {
            switch (*rest) {
                case 'K': board.castling_rights |= 0b0001; break;
                case 'Q': board.castling_rights |= 0b0010; break;
                case 'k': board.castling_rights |= 0b0100; break;
                case 'q': board.castling_rights |= 0b1000; break;
            }
        }
    }
    if (*rest == ' ') rest++;

    // 4. Parse en passant square
    if (*rest == '-') {
        board.enpassant_square = 64;
        rest++;
    } else {
        int file = rest[0] - 'a';
        int rank = rest[1] - '1';
        board.enpassant_square = rank * 8 + file;
        rest += 2;
    }
    if (*rest == ' ') rest++;

    // 5. Parse halfmove clock
    board.half_move_clock = atoi(rest);
    while (*rest && *rest != ' ') rest++;
    if (*rest == ' ') rest++;

    // 6. Parse fullmove number
    board.full_moves = atoi(rest);
}

void parse_and_export_fen_params(std::string fen) {
    // Side to move
    fen += (board.side_to_move == 0) ? " w " : " b ";

    // Castling rights
    std::string castling;
    if (board.castling_rights & 0b1000) castling += 'K';
    if (board.castling_rights & 0b0100) castling += 'Q';
    if (board.castling_rights & 0b0010) castling += 'k';
    if (board.castling_rights & 0b0001) castling += 'q';
    if (castling.empty()) castling = "-";
    fen += castling + " ";

    // En passant
    if (board.enpassant_square < 64) {
        fen += square_idx_to_alg(board.enpassant_square);
    } else {
        fen += "-";
    }
    fen += " ";

    // Halfmove clock
    fen += std::to_string(board.half_move_clock);
    fen += " ";

    // Fullmove number
    fen += std::to_string(board.full_moves);
}
