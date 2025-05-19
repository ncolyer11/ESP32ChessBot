#include "board.h"
#include "piece.h"
#include <bitset>
#include <cstring>
#include <cctype>
#include <iostream>
#include <string>

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
            uint8_t piece = board.squares[rank * 8 + file];
            char symbol = PIECE_SYMBOLS[piece & TYPE_MASK];
            if (piece & BLACK_MASK) symbol = tolower(symbol);
            std::cout << symbol << ' ';
        }
        std::cout << std::endl;
    }
    std::cout << "  a b c d e f g h\n" << std::endl;

    // Show additional parameters if requested after printin
    if (show_params) {
        std::cout << "Side to move: " << (board.side_to_move == 0 ? "White" : "Black") << std::endl;
        std::cout << "Castling rights: ";
        if (board.castling_rights & 0b1000) std::cout << 'K';
        if (board.castling_rights & 0b0100) std::cout << 'Q';
        if (board.castling_rights & 0b0010) std::cout << 'k';
        if (board.castling_rights & 0b0001) std::cout << 'q';
        if (board.castling_rights == 0) std::cout << '-';
        std::cout << std::endl;
        std::cout << "En passant square: " << (board.enpassant_square < 64 ? board.enpassant_square : '-') << std::endl;
        std::cout << "Halfmove clock: " << static_cast<int>(board.half_move_clock) << std::endl;
        std::cout << "Fullmove number: " << board.full_moves << std::endl;
    }

    // Show bitboards if requested
    if (show_bitboards) {
        std::cout << "Bitboards:" << std::endl;
        for (int i = 0; i < 12; ++i) {
            std::cout << "Piece " << i << ": " << std::bitset<64>(board.bitboards[i]) << std::endl;
        }
    }
}

void load_position_from_fen(const char* fen) {
    // Clear old Board state
    std::memset(board.squares, NONE, sizeof(board.squares));
    std::memset(board.bitboards, 0, sizeof(board.bitboards));

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
            uint8_t piece;
            switch (tolower(symbol)) {
                case 'k': piece = KING; break;
                case 'p': piece = PAWN; break;
                case 'n': piece = KNIGHT; break;
                case 'b': piece = BISHOP; break;
                case 'r': piece = ROOK; break;
                case 'q': piece = QUEEN; break;
                default: piece = NONE; break;
            }

            if (piece != NONE) piece |= (isupper(symbol) ? WHITE_MASK : BLACK_MASK);
            uint8_t square_index = rank * 8 + file;
            // 64-bit mask with 1 at the square position, 1ULL is a 64-bit unsigned long long
            uint64_t square_mask = 1ULL << square_index; 
            board.squares[square_index] = piece;
            uint8_t bitboard_index = (piece & TYPE_MASK) + (piece & WHITE_MASK ? 0 : 6);
            board.bitboards[bitboard_index] |= square_mask;
            file++;
        }
    }
    // Parse the rest of the FEN string
    parse_fen_flags(fen, fen_board);
}

void parse_fen_flags(const char* fen, char *fen_board) {
    // After parsing board layout, parse rest of FEN:
    const char* rest = fen + strlen(fen_board);
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
                case 'K': board.castling_rights |= 0b1000; break;
                case 'Q': board.castling_rights |= 0b0100; break;
                case 'k': board.castling_rights |= 0b0010; break;
                case 'q': board.castling_rights |= 0b0001; break;
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

std::string export_position_to_fen() {
    std::string fen;
    for (int rank = 7; rank >= 0; --rank) {
        int empty_count = 0;
        for (int file = 0; file < 8; ++file) {
            uint8_t piece = board.squares[rank * 8 + file];
            if (piece == NONE) {
                empty_count++;
            } else {
                if (empty_count > 0) {
                    fen += std::to_string(empty_count);
                    empty_count = 0;
                }
                int piece_index = (piece & TYPE_MASK) + (piece & WHITE_MASK ? 0 : 6);
                char symbol = PIECE_SYMBOLS[piece_index];
                symbol = (piece & BLACK_MASK) ? tolower(symbol) : toupper(symbol);
                fen += symbol;
            }
        }
        if (empty_count > 0) fen += std::to_string(empty_count);
        if (rank != 0) fen += '/';
    }

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
        int rank_num = board.enpassant_square / 8;
        int file_num = board.enpassant_square % 8;
        if (rank_num % 2) file_num = 7 - file_num; // Flip file for snakes & ladders pattern
        char pos = 'a' + file_num + '1' + rank_num;
        fen += pos;
    } else {
        fen += "-";
    }
    fen += " ";

    // Halfmove clock
    fen += std::to_string(board.half_move_clock);
    fen += " ";

    // Fullmove number
    fen += std::to_string(board.full_moves);

    return fen;
}
