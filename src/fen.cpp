#include <bitset>
#include <cstring>
#include <cctype>
#include <iostream>
#include <limits>
#include <string>

#include "board.h"
#include "boardRepresentation.h"
#include "piece.h"
#include "fen.h"
#include "move.h"
#include "util.h"

const std::string starting_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
static const std::string check_fen = "4k3/8/2p5/2P5/8/2b5/1N3P2/K6N w - - 0 1";
static const std::string stalemate_fen = "7k/2p5/5N2/2P5/8/5R2/1N3P2/K7 b - - 0 1";
static const std::string queens_fen = "rnbqkbnr/qpqqpqpp/7p/8/8/P7/PQQPPQQQ/RNBQKBNR w KQkq - 0 1";
static const std::string coverage_fen = "r3kb1r/pp3bP1/3n4/1r6/1n1p4/R7/pKPQ1p1q/8 w q - 0 1";
static const std::string castling_fen = "r3k1r1/1ppppp2/3r1p2/8/8/4PP2/1PP1PP2/R3K2R w KQkq - 0 1";
static const std::string checkmate_fen = "k7/pp5R/8/8/8/8/8/K7 w - - 0 1";
static const std::string take_out_of_check_fen = "1k6/ppp5/8/2b5/8/4BN2/1P2QPPP/5RK1 w - - 0 1";
static const std::string rook_only_cm_fen = "6k1/8/8/3P4/2K5/8/8/8 b - - 2 73";

// Hand-written FEN check (no std::regex: it is huge on ESP32). The last two fields are optional.
bool is_valid_fen(const std::string &fen) {
    size_t i = 0, n = fen.size();
    int wk = 0, bk = 0;
    for (int rank = 0; rank < 8; rank++) {
        int files = 0;
        for (; i < n && fen[i] != '/' && fen[i] != ' '; i++) {
            char c = fen[i];
            if (c >= '1' && c <= '8') {
                files += c - '0';
            } else if (strchr("pnbrqkPNBRQK", c) && c != '\0') {
                if ((c == 'p' || c == 'P') && (rank == 0 || rank == 7)) return false;
                if (c == 'K') wk++;
                if (c == 'k') bk++;
                files++;
            } else {
                return false;
            }
            if (files > 8) return false;
        }
        if (files != 8) return false;
        if (rank < 7) {
            if (i >= n || fen[i] != '/') return false;
            i++;
        }
    }
    if (wk != 1 || bk != 1) return false;
    if (i >= n || fen[i] != ' ') return false;
    i++;
    if (i >= n || (fen[i] != 'w' && fen[i] != 'b')) return false;
    i++;
    if (i >= n || fen[i] != ' ') return false;
    i++;
    if (i < n && fen[i] == '-') {
        i++;
    } else {
        int cnt = 0;
        for (; i < n && fen[i] != ' '; i++, cnt++) if (!strchr("KQkq", fen[i])) return false;
        if (cnt == 0 || cnt > 4) return false;
    }
    if (i >= n || fen[i] != ' ') return false;
    i++;
    if (i < n && fen[i] == '-') {
        i++;
    } else {
        if (i + 1 >= n || fen[i] < 'a' || fen[i] > 'h' || (fen[i + 1] != '3' && fen[i + 1] != '6')) return false;
        i += 2;
    }
    // optional halfmove clock and fullmove number
    for (int field = 0; field < 2 && i < n; field++) {
        if (fen[i] != ' ') return false;
        i++;
        size_t start = i;
        for (; i < n && isdigit((unsigned char)fen[i]); i++) {}
        if (i == start) return false;
    }
    return i == n;
}

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
bool load_position_from_fen(Board *b, std::string fen) {
    bool valid = is_valid_fen(fen);
    if (!valid) {
        std::cout << "Invalid FEN: " << fen << "\nUsing default starting position." << std::endl;
        fen = starting_fen;
    }

    // Reset all state
    std::memset(b->squares, NONE, sizeof(b->squares));
    std::memset(b->bitboards, 0, sizeof(b->bitboards));
    b->undo_state_stack.clear();
    b->hash_history.clear();
    b->enpassant_square = 64;
    b->half_move_clock = 0;
    b->full_moves = 1;
    move_history.clear();
    // reserve up front so long games don't fragment the heap on small targets
    constexpr size_t HISTORY_RESERVE = 512;
    b->undo_state_stack.reserve(HISTORY_RESERVE);
    b->hash_history.reserve(HISTORY_RESERVE);
    move_history.reserve(HISTORY_RESERVE);

    std::cout << "Loading position from FEN: " << fen << std::endl;
    int file = 0, rank = 7;

    // Only take board layout (before first space)
    size_t board_len = fen.find(' ');
    if (board_len == std::string::npos) board_len = fen.size();

    for (size_t idx = 0; idx < board_len; idx++) {
        char symbol = fen[idx];
        if (symbol == '/') {
            file = 0;
            rank--;
        } else if (isdigit(symbol)) {
            file += symbol - '0';
        } else {
            Piece piece = symbol_to_piece(symbol);
            uint8_t square_idx = file_rank_to_square_idx(file, rank);
            b->squares[square_idx] = piece;
            b->bitboards[piece_to_bb_idx(piece)] |= 1ULL << square_idx;
            file++;
        }
    }

    // Parse the rest of the FEN parameters
    parse_and_load_fen_params(b, fen.c_str() + board_len);
    b->zobrist = compute_zobrist(b);
    return valid;
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

    return fen; // validated (or replaced by the default) in load_position_from_fen
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

    // 5. Parse halfmove clock (optional)
    b->half_move_clock = *rest ? atoi(rest) : 0;
    while (*rest && *rest != ' ') rest++;
    if (*rest == ' ') rest++;

    // 6. Parse fullmove number (optional)
    b->full_moves = *rest ? atoi(rest) : 1;
    if (b->full_moves < 1) b->full_moves = 1;
}

std::string parse_and_export_fen_params(Board *b) {
    std::string fen_params;

    // Side to move
    fen_params += (b->side_to_move == 0) ? " w " : " b ";

    // Castling rights
    std::string castling;
    if (b->castling_rights & 0b0001) castling += 'K';
    if (b->castling_rights & 0b0010) castling += 'Q';
    if (b->castling_rights & 0b0100) castling += 'k';
    if (b->castling_rights & 0b1000) castling += 'q';
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
