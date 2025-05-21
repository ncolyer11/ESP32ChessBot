#include <bitset>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "board.h"
#include "boardRepresentation.h"
#include "move.h"
#include "movegen.h"
#include "piece.h"
#include "search.h"
#include "util.h"

// Output arrays
Move move_list[MAX_MOVES];
int move_count = 0;
Move legal_move_list[MAX_MOVES];
int legal_move_count = 0;

// Precomputed attack bitboards
uint64_t pawn_attacks[2][64];
uint64_t knight_attacks[64];
uint64_t king_attacks[64];

Move gen_and_validate_all_moves();

// Request next move from the current player
Move get_next_player_move() {
    if (gen_and_validate_all_moves() == NO_MOVES) return NO_MOVES;

    Move move;
    do {
        std::string move_str;
        std::cout << ((board.side_to_move == 0) ? "White" : "Black") << " to move: ";
        if (!(std::cin >> move_str)) {  // exit on EOF (ctrl + z + enter on Windows *apparently*)
            std::cout << "Game ended prematurely by user." << std::endl;
            return INVALID_MOVE;
        }
        move = move_str_to_move(move_str);
    } while (!is_valid_move(&move));

    return move;
}

bool is_valid_move(Move *move) {
    for (int i = 0; i < legal_move_count; ++i) {
        int flag = move_flag(*move);
        Move legal_move = legal_move_list[i];
        if (!(flag >= 3 && flag <= 6)) legal_move &= 0x0FFF; // clear the last 4 bits/flag
        if (*move == legal_move) {
            *move = legal_move_list[i]; // update move with flags
            return true;
        }
    }

    std::cout << "Invalid move: " << move_to_move_str(*move) << std::endl;
    return false;
}

/**
 * @brief Generates a list of all legal moves for the current board position, using the global 'board'.
 * 
 * Globals Used:
 *   - board (input): The current board state for which moves are generated.
 *   - move_list, move_count (output): Filled with all pseudo-legal moves found.
 *   - legal_move_list, legal_move_count (output): Populated with all legal moves after filtering.
 *
 * Side Effects:
 *   - Updates move_list, move_count, legal_move_list, and legal_move_count with current move information.
 * 
 * Usage:
 *   Call generate_moves() before move selection or perft, and after making any board changes.
 */
void generate_moves() {
    // reset move count and both move and legal move lists
    move_count = 0;
    std::memset(move_list, 0, sizeof(move_list));
    std::memset(legal_move_list, 0, sizeof(legal_move_list));
    
    generate_king_moves();
    generate_castling_moves();
    generate_sliding_moves(); // Queen, rook, bishop
    generate_knight_moves();
    generate_pawn_moves();
}

// -- Legal move filtering --
// Writes all legal moves from the global move_list/move_count into legal_move_list/legal_move_count
void filter_legal_moves() {
    legal_move_count = 0;
    for (int i = 0; i < move_count; i++) {
        Board temp = board; // copy the global board
        std::string move_str = move_to_move_str(move_list[i]);
        // std::cout << "[filter_legal_moves] Testing move: " << move_str << std::endl;
        make_move_on_board(&temp, move_list[i]);
        // Pass in original side_to_move, as it gets inverted when make_move_on_board is called
        if (!king_in_check(&temp, board.side_to_move)) {
            // std::cout << "[filter_legal_moves] Move is legal: " << move_str << std::endl;
            legal_move_list[legal_move_count++] = move_list[i];
        } else {
            // std::cout << "[filter_legal_moves] Move is illegal (king in check): " << move_str << std::endl;
        }
        // std::cout << std::endl;
    }
    // std::cout << "[filter_legal_moves] Total legal moves: " << legal_move_count << std::endl;
}


// Returns a heap-allocated copy of the current legal move list and its count.
// Caller is responsible for deleting the returned moves array.
MoveList generate_moves_arr() {
    generate_moves();
    MoveList result;
    result.count = legal_move_count;
    result.moves = new Move[legal_move_count];
    std::memcpy(result.moves, legal_move_list, legal_move_count * sizeof(Move));
    return result;
}

// Attack table initialisation, saying at square sq, you have attacks...
void init_attack_bitboards() {
    // Knights
    for (int sq = 0; sq < 64; sq++) {
        knight_attacks[sq] = 0;
        int r = sq / 8, f = sq % 8;
        const int dr[8] = {1, 2, 2, 1, -1, -2, -2, -1};
        const int df[8] = {2, 1, -1, -2, -2, -1, 1, 2};
        for (int d = 0; d < 8; d++) {
            int nr = r + dr[d], nf = f + df[d];
            if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8)
                knight_attacks[sq] |= 1ULL << (nr*8+nf);
        }
    }
    // Kings
    for (int sq = 0; sq < 64; sq++) {
        king_attacks[sq] = 0;
        int rank = sq / 8, file = sq % 8;
        for (int dr = -1; dr <= 1; dr++) {
            for (int df = -1; df <= 1; df++) {
                if (dr == 0 && df == 0) continue;
                int nr = rank + dr, nf = file + df;
                if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8)
                    king_attacks[sq] |= 1ULL << (nr*8+nf);
            }
        }
    }
    // Pawns
    for (int sq = 0; sq < 64; sq++) {
        pawn_attacks[0][sq] = 0; // White
        pawn_attacks[1][sq] = 0; // Black
        int r = sq / 8, f = sq % 8;
        // White
        if (r < 7) {
            if (f > 0) pawn_attacks[0][sq] |= 1ULL << ((r+1)*8 + (f-1));
            if (f < 7) pawn_attacks[0][sq] |= 1ULL << ((r+1)*8 + (f+1));
        }
        // Black
        if (r > 0) {
            if (f > 0) pawn_attacks[1][sq] |= 1ULL << ((r-1)*8 + (f-1));
            if (f < 7) pawn_attacks[1][sq] |= 1ULL << ((r-1)*8 + (f+1));
        }
    }
}

// -- King moves (not castling) --
void generate_king_moves() {
    int king_sq = -1;
    Piece king_piece = KING | friendly_colour();
    // TODO: use the magic lookup de bruj shit instead here
    for (int i = 0; i < 64; i++) {
        if (board.squares[i] == king_piece) {
            king_sq = i;
            break;
        }
    }
    if (king_sq == -1) return;

    uint64_t king_targets = king_attacks[king_sq];
    for (int t = 0; t < 64; t++) {
        if ((king_targets & (1ULL << t)) && 
            (!is_friendly(board.squares[t]) || board.squares[t] == NONE)) {

            add_move(king_sq, t, 0);
        }
    }
}

// -- Castling moves (call after king moves) --
void generate_castling_moves() {
    int king_sq = (friendly_colour() == WHITE) ? 4 : 60;
    // std::cout << "Castling rights: " << std::bitset<4>(board.castling_rights) << std::endl;
    // Kingside
    if ((friendly_colour() == WHITE && (board.castling_rights & 0b0001)) ||
        (friendly_colour() == BLACK && (board.castling_rights & 0b0100))) {
        int f = (friendly_colour() == WHITE) ? 5 : 61;
        int g = (friendly_colour() == WHITE) ? 6 : 62;

        if (board.squares[f] == NONE && board.squares[g] == NONE &&
            !king_in_check(&board, board.side_to_move) &&
            !king_in_check_after_move(king_sq, f) &&
            !king_in_check_after_move(king_sq, g)) {
            add_move(king_sq, g, 2); // Castling flag
        }
    }

    // Queenside
    if ((friendly_colour() == WHITE && (board.castling_rights & 0b0010)) ||
        (friendly_colour() == BLACK && (board.castling_rights & 0b1000))) {
        int d = (friendly_colour() == WHITE) ? 3 : 59;
        int c = (friendly_colour() == WHITE) ? 2 : 58;
        int b = (friendly_colour() == WHITE) ? 1 : 57;

        if (board.squares[d] == NONE && board.squares[c] == NONE && board.squares[b] == NONE &&
            !king_in_check(&board, board.side_to_move) &&
            !king_in_check_after_move(king_sq, d) &&
            !king_in_check_after_move(king_sq, c)) {
            add_move(king_sq, c, 2); // Castling flag
        }
    }
}


// -- Sliding piece moves (rooks, bishops, queens) --
void generate_sliding_moves() {
    static const int directions[8] = {1, -1, 8, -8, 9, -9, 7, -7};
    for (int i = 0; i < 64; i++) {
        Piece p = board.squares[i];
        if (!is_friendly(p)) continue;
        int type = p & TYPE_MASK;
        int start_dir = (type == ROOK) ? 0 : (type == BISHOP) ? 4 : 0;
        int end_dir = (type == ROOK) ? 4 : (type == BISHOP) ? 8 : (type == QUEEN) ? 8 : 0;
        if (type != ROOK && type != BISHOP && type != QUEEN) continue;
        for (int d = start_dir; d < end_dir; ++d) {
            int n = i;
            while (true) {
                int next = n + directions[d];
                if (next < 0 || next >= 64 || abs((n % 8) - (next % 8)) > 2) break;
                Piece target = board.squares[next];
                if (is_friendly(target)) break;
                add_move(i, next, 0);
                if (target != NONE) break;
                n = next;
            }
        }
    }
}

// -- Knight moves --
void generate_knight_moves() {
    for (int i = 0; i < 64; i++) {
        if (board.squares[i] != (KNIGHT | friendly_colour())) continue;
        uint64_t attacks = knight_attacks[i];
        for (int t = 0; t < 64; t++)
            if ((attacks & (1ULL << t)) && !is_friendly(board.squares[t]))
                add_move(i, t, 0);
    }
}

// -- Pawn moves (single, double, capture, en passant, promotion) --
void generate_pawn_moves() {
    int direction = board.side_to_move ? -8 : 8;
    int start_rank = board.side_to_move ? 6 : 1;
    int promotion_rank = board.side_to_move ? 0 : 7;
    // std::cout << "turn: " << board.side_to_move << std::endl;
    for (int sq_idx = 0; sq_idx < 64; sq_idx++) {
        if (board.squares[sq_idx] != (PAWN | friendly_colour())) continue;
        // std::cout << "Pawn found at position: " << sq_idx << std::endl;
        int to = sq_idx + direction;
        // Forward
        if (to >= 0 && to < 64 && board.squares[to] == NONE) {
            if (to / 8 == promotion_rank) {
                // Add the 4 different possible promotion moves
                add_move(sq_idx, to, 3); add_move(sq_idx, to, 4);
                add_move(sq_idx, to, 5); add_move(sq_idx, to, 6);
            } else {
                // Else it can't promote but still check if it can double push
                add_move(sq_idx, to, 0);
                // std::cout << "Pawn single push from " << sq_idx << " to " << to << std::endl;
                // Double push from start
                if (sq_idx / 8 == start_rank) {
                    int dbl = sq_idx + 2 * direction;
                    if (board.squares[dbl] == NONE) {
                        add_move(sq_idx, dbl, 7);
                        // std::cout << "Pawn double push from " << sq_idx << " to " << dbl << std::endl;
                    }
                }
            }
        }
        // Captures
        for (int df = -1; df <= 1; df += 2) {
            int file = sq_idx % 8;
            int cap_file = file + df;
            if (cap_file < 0 || cap_file > 7) continue;  // Add this check!
            int cap = sq_idx + direction + df;
            if (cap < 0 || cap >= 64) continue;
            if (board.squares[cap] != NONE && is_opponent(board.squares[cap])) {
                if (cap / 8 == promotion_rank) {
                    add_move(sq_idx, cap, 3); add_move(sq_idx, cap, 4);
                    add_move(sq_idx, cap, 5); add_move(sq_idx, cap, 6);
                } else {
                    add_move(sq_idx, cap, 0);
                }
            }
        }
        // En passant
        if (board.enpassant_square < 64) {
            int file = sq_idx % 8;
            for (int df = -1; df <= 1; df += 2) {
                int cap_file = file + df;
                if (cap_file < 0 || cap_file > 7) continue;
                int cap = sq_idx + direction + df;
                if (cap == board.enpassant_square) {
                    add_move(sq_idx, cap, 1);
                }
            }
        }
    }
}

// Check if a king is attacked after moving to a square
bool king_in_check_after_move(int from, int to) {
    Board temp = board;
    make_move_on_board(&temp, encode_move(from, to, 0));
    bool in_check = king_in_check(&temp, board.side_to_move);
    return in_check;
}

bool check_end_game_conditions(Board *b, int legal_moves_count) {
    bool is_king_in_check = king_in_check(b, b->side_to_move);
    if (is_checkmate(legal_moves_count, is_king_in_check)) {
        print_board(true);
        std::cout << (b->side_to_move == 0 ? "White" : "Black") << " is checkmated. "
                  << (b->side_to_move == 1 ? "White" : "Black") << " wins." << std::endl;
        return false;
    }

    if (is_draw(b->half_move_clock)) {
        print_board(true);
        std::cout << "Draw by fifty-move rule!" << std::endl;
        return false;
    }

    if (is_stalemate(legal_moves_count, is_king_in_check)) {
        print_board(true);
        std::cout << "Stalemate!" << std::endl;
        return false;
    }

    std::cout << "[DEBUG] Endgame check: side_to_move=" << b->side_to_move
              << ", legal_moves_count=" << legal_moves_count
              << ", is_king_in_check=" << is_king_in_check << std::endl;
    return true;
}


bool is_checkmate(int legal_moves_count, bool is_king_in_check) {
    return (legal_moves_count == 0) && is_king_in_check;
}

bool is_draw(uint8_t half_move_clock) {
    return half_move_clock > 50;
}

bool is_stalemate(int legal_moves_count, bool is_king_in_check) {
    return (legal_moves_count == 0) && !is_king_in_check;
}

Move gen_and_validate_all_moves() {
    generate_moves();
    filter_legal_moves();

    if (legal_move_count == 0) {
        std::cout << "Chesp32 (" << (board.side_to_move == 0 ? "White" : "Black") << ") has no legal moves." << std::endl;
        return NO_MOVES;
    } else if (board.half_move_clock > 50) {
        // Error message printed in check_end_game_conditions()
        return NO_MOVES;
    }

    return 1;
}
