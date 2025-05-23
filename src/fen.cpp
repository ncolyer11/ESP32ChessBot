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

void parse_and_load_fen_params(Board *b, const char *rest);
std::string parse_and_export_fen_params(Board *b);

/**
 * @brief Loads the board position from a FEN string.
 *
 * The FEN string should be in standard format:
 *   "piece_placements active_color castling_availability en_passant_square halfmove_clock fullmove"
 * For example: "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
 *
 * @param fen The FEN string describing the board state.
 */
void load_position_from_fen(Board *b, std::string fen) {
    // Clear old Board state
    std::memset(b->squares, NONE, sizeof(b->squares));
    std::memset(b->bitboards, 0, sizeof(b->bitboards));
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
            b->squares[square_idx] = piece;
            b->bitboards[piece_to_bb_idx(piece)] |= square_mask;
            file++;
        }
    }

    // Parse the rest of the FEN parameters
    parse_and_load_fen_params(b, fen.c_str() + strlen(fen_board));
}

/**
 * @brief Exports the current board state as a FEN string.
 * @return The FEN string representation of the b->
 */
std::string export_position_to_fen(Board *b) {
    std::string fen;
    for (int rank = 7; rank >= 0; rank--) {
        int empty_count = 0;
        for (int file = 0; file < 8; file++) {
            Piece piece = b->squares[rank * 8 + file];
            if ((piece & TYPE_MASK) == NONE) {
                empty_count++;
            } else {
                if (empty_count > 0) {
                    fen += std::to_string(empty_count);
                    empty_count = 0;
                }
                char symbol = PIECE_SYMBOLS[piece & TYPE_MASK];
                symbol = (piece & BLACK) ? tolower(symbol) : toupper(symbol);
                fen += symbol;
            }
        }
        if (empty_count > 0) fen += std::to_string(empty_count);
        if (rank != 0) fen += '/';
    }

    // Parse and export the FEN parameters
    return fen + parse_and_export_fen_params(b);
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
        if (fen == "checkmate") return checkmate_fen;
        if (fen == "takecheck") return take_out_of_check_fen;
        if (fen == "rookcm") return rook_only_cm_fen;
    }

    if (std::regex_match(fen, fen_regex)) return fen;
    
    std::cout << "Invalid FEN format. Using default starting position." << std::endl;
    return starting_fen;
}

void ask_export_fen(Board *b) {
    std::cout << "Export final position to FEN? (Y/n): ";
    std::string input;
    // std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // flush remaining newline
    std::getline(std::cin, input);
    if (input.empty() || tolower(input[0]) == 'y') {
        std::cout << "FEN: " << export_position_to_fen(b) << std::endl;
    }
}

// After parsing board layout, parse rest of FEN:
void parse_and_load_fen_params(Board *b, const char *rest) {
    if (*rest == ' ') rest++;

    // 2. Parse side to move
    b->side_to_move = (*rest == 'w') ? 0 : 1;
    while (*rest && *rest != ' ') rest++; // skip to next field
    if (*rest == ' ') rest++;

    // 3. Parse castling rights
    b->castling_rights = 0;
    if (*rest == '-') {
        rest++;
    } else {
        for (; *rest && *rest != ' '; rest++) {
            switch (*rest) {
                case 'K': b->castling_rights |= 0b0001; break;
                case 'Q': b->castling_rights |= 0b0010; break;
                case 'k': b->castling_rights |= 0b0100; break;
                case 'q': b->castling_rights |= 0b1000; break;
            }
        }
    }
    if (*rest == ' ') rest++;

    // 4. Parse en passant square
    if (*rest == '-') {
        b->enpassant_square = 64;
        rest++;
    } else {
        int file = rest[0] - 'a';
        int rank = rest[1] - '1';
        b->enpassant_square = rank * 8 + file;
        rest += 2;
    }
    if (*rest == ' ') rest++;

    // 5. Parse halfmove clock
    b->half_move_clock = atoi(rest);
    while (*rest && *rest != ' ') rest++;
    if (*rest == ' ') rest++;

    // 6. Parse fullmove number
    b->full_moves = atoi(rest);
}

std::string parse_and_export_fen_params(Board *b) {
    std::string fen_params;

    // Side to move
    fen_params += (b->side_to_move == 0) ? " w " : " b ";

    // Castling rights
    std::string castling;
    if (b->castling_rights & 0b1000) castling += 'K';
    if (b->castling_rights & 0b0100) castling += 'Q';
    if (b->castling_rights & 0b0010) castling += 'k';
    if (b->castling_rights & 0b0001) castling += 'q';
    if (castling.empty()) castling = "-";
    fen_params += castling + " ";

    // En passant
    if (b->enpassant_square < 64) {
        fen_params += square_idx_to_alg(b->enpassant_square);
    } else {
        fen_params += "-";
    }
    fen_params += " ";

    // Halfmove clock
    fen_params += std::to_string(b->half_move_clock);
    fen_params += " ";

    // Fullmove number
    fen_params += std::to_string(b->full_moves);

    return fen_params;
}
