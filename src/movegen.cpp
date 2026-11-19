#include <bitset>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <algorithm>

#include "board.h"
#include "boardRepresentation.h"
#include "fen.h"
#include "move.h"
#include "movegen.h"
#include "piece.h"
#include "chess_search.h"
#include "util.h"

// Precomputed attack bitboards
uint64_t pawn_attacks[2][64];
uint64_t knight_attacks[64];
uint64_t king_attacks[64];

Move gen_and_validate_all_moves(Board *b, MoveList *legal_move_list);

enum CommandResult { CMD_NOT_A_COMMAND, CMD_HANDLED, CMD_QUIT, CMD_UNDONE };
static CommandResult process_command_moves(Board *b, std::string move_str);
static void print_help();
static bool undo_plies(Board *b, int plies);

// Request next move from the current player
Move get_next_player_move(Board *b) {
    // Check if the player has any available moves
    MoveList legal_move_list = { {}, 0 };
    if (gen_and_validate_all_moves(b, &legal_move_list) == NO_MOVES) return NO_MOVES;

    while (true) {
        std::string move_str;
        std::cout << ((b->side_to_move == 0) ? "White" : "Black") << " to move: ";
        if (!(std::cin >> move_str)) {  // exit on EOF (ctrl + z + enter on Windows *apparently*)
            std::cout << "Game ended prematurely by user." << std::endl;
            return INVALID_MOVE;
        }

        CommandResult cmd = process_command_moves(b, move_str);
        if (cmd == CMD_QUIT) return INVALID_MOVE;
        if (cmd == CMD_UNDONE) return UNDO_DONE;
        if (cmd == CMD_HANDLED) continue;

        Move move = move_str_to_move(move_str);
        if (move != INVALID_MOVE && is_valid_move(&move, legal_move_list)) return move;
        if (move == INVALID_MOVE) std::cout << "Invalid move: " << move_str << " (type h for help)" << std::endl;
    }
}

static void print_help() {
    std::cout << "Enter a move like e2e4 (e7e8q to promote, queen by default) or a command:\n"
              << "  u  undo back to your last turn (2 plies vs the bot, 1 in 2 player games)\n"
              << "  z  undo the last 2 plies\n"
              << "  f  print FEN        d  draw board from bitboards\n"
              << "  p  print params     b  print bitboards\n"
              << "  r  resign           q  quit\n"
              << "  h  this help" << std::endl;
}

// Undo the given number of plies if there is that much history
static bool undo_plies(Board *b, int plies) {
    if ((int)move_history.size() < plies) {
        std::cout << "Nothing to undo." << std::endl;
        return false;
    }
    std::cout << "Undoing " << plies << (plies == 1 ? " ply" : " plies") << ":";
    for (int i = 0; i < plies; i++) {
        Move last_move = move_history.back();
        std::cout << " " << move_to_move_str(last_move);
        unmake_move_on_board(b, last_move);
    }
    std::cout << std::endl;
    return true;
}

static CommandResult process_command_moves(Board *b, std::string move_str) {
    // Convert move_str to lowercase for easier comparison
    std::transform(move_str.begin(), move_str.end(), move_str.begin(), ::tolower);
    if (move_str == "q") {
        std::cout << "Game ended prematurely by user." << std::endl;
        return CMD_QUIT;
    } else if (move_str == "r") {
        std::cout << (b->side_to_move == 0 ? "White" : "Black") << " resigns. "
                  << (b->side_to_move == 0 ? "Black" : "White") << " wins." << std::endl;
        return CMD_QUIT;
    } else if (move_str == "u") {
        // vs a bot, go back to the human's previous turn (skip the bot's reply too)
        bool opponent_is_bot = !player_types[b->side_to_move ^ 1];
        return undo_plies(b, opponent_is_bot ? 2 : 1) ? CMD_UNDONE : CMD_HANDLED;
    } else if (move_str == "z") {
        return undo_plies(b, 2) ? CMD_UNDONE : CMD_HANDLED;
    } else if (move_str == "f") {
        std::cout << "Exporting FEN: " << export_position_to_fen(b) << std::endl;
        return CMD_HANDLED;
    } else if (move_str == "d") {
        print_board_from_bbs(b, true);
        return CMD_HANDLED;
    } else if (move_str == "p") {
        print_params(b);
        return CMD_HANDLED;
    } else if (move_str == "b") {
        print_bitboards(b);
        return CMD_HANDLED;
    } else if (move_str == "h" || move_str == "help" || move_str == "?") {
        print_help();
        return CMD_HANDLED;
    }

    return CMD_NOT_A_COMMAND;
}

bool is_valid_move(Move *move, const MoveList &legal_move_list) {
    for (int i = 0; i < legal_move_list.count; ++i) {
        int flag = move_flag(*move);
        Move legal_move = legal_move_list.moves[i];
        if (!(flag >= 3 && flag <= 6)) legal_move &= 0x0FFF; // clear the last 4 bits/flag
        if (*move == legal_move) {
            *move = legal_move_list.moves[i]; // update move with flags
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
void generate_moves(Board *b, MoveList *legal_move_list, bool only_captures) {
    // reset move count and both move and legal move lists
    MoveList move_list = { {}, 0 };
    
    generate_king_moves(b, &move_list, only_captures);
    generate_castling_moves(b, &move_list, only_captures);
    generate_sliding_moves(b, &move_list, only_captures); // Queen, rook, bishop
    generate_knight_moves(b, &move_list, only_captures);
    generate_pawn_moves(b, &move_list, only_captures);

    filter_legal_moves(b, move_list, legal_move_list);
}

// -- Legal move filtering --
void filter_legal_moves(Board *b, const MoveList &move_list, MoveList *legal_move_list) {
    legal_move_list->count = 0;
    for (int i = 0; i < move_list.count; i++) {
        if (!get_check(b, move_list.moves[i], 101)) {
            legal_move_list->moves[legal_move_list->count++] = move_list.moves[i];
        }
    }
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

static inline U64 friendly_occupancy(Board *b) {
    int c = b->side_to_move ? 6 : 0;
    return b->bitboards[c] | b->bitboards[c + 1] | b->bitboards[c + 2] |
           b->bitboards[c + 3] | b->bitboards[c + 4] | b->bitboards[c + 5];
}
static inline U64 opponent_occupancy(Board *b) {
    int c = b->side_to_move ? 0 : 6;
    return b->bitboards[c] | b->bitboards[c + 1] | b->bitboards[c + 2] |
           b->bitboards[c + 3] | b->bitboards[c + 4] | b->bitboards[c + 5];
}

// -- King moves (not castling) --
void generate_king_moves(Board *b, MoveList *move_list, bool only_captures) {
    Piece king_piece = KING | friendly_colour(b);
    U64 king_bb = b->bitboards[piece_to_bb_idx(king_piece)];
    if (!king_bb) return; // No king on the board
    int king_sq = bit_scan_forward(king_bb);

    U64 targets = king_attacks[king_sq] & ~friendly_occupancy(b);
    if (only_captures) targets &= opponent_occupancy(b);
    while (targets) {
        add_move(move_list, king_sq, bit_scan_forward(targets), 0);
        targets &= targets - 1;
    }
}

// -- Castling moves (call after king moves) --
void generate_castling_moves(Board *b, MoveList *move_list, bool only_captures) {
    if (only_captures) return; // No castling moves if only captures are requested
    int king_sq = (friendly_colour(b) == WHITE) ? 4 : 60;
    // std::cout << "Castling rights: " << std::bitset<4>(b->castling_rights) << std::endl;
    // Kingside
    if ((friendly_colour(b) == WHITE && (b->castling_rights & 0b0001)) ||
        (friendly_colour(b) == BLACK && (b->castling_rights & 0b0100))) {
        int f = (friendly_colour(b) == WHITE) ? 5 : 61;
        int g = (friendly_colour(b) == WHITE) ? 6 : 62;

        if (b->squares[f] == NONE && b->squares[g] == NONE &&
            !king_in_check(b) &&
            !king_in_check_after_move(b, king_sq, f) &&
            !king_in_check_after_move(b, king_sq, g)) {
            add_move(move_list, king_sq, g, CASTLING_FLAG);
        }
    }

    // Queenside
    if ((friendly_colour(b) == WHITE && (b->castling_rights & 0b0010)) ||
        (friendly_colour(b) == BLACK && (b->castling_rights & 0b1000))) {
        int d = (friendly_colour(b) == WHITE) ? 3 : 59;
        int c = (friendly_colour(b) == WHITE) ? 2 : 58;
        int b_rank = (friendly_colour(b) == WHITE) ? 1 : 57;

        if (b->squares[d] == NONE && b->squares[c] == NONE && b->squares[b_rank] == NONE &&
            !king_in_check(b) &&
            !king_in_check_after_move(b, king_sq, d) &&
            !king_in_check_after_move(b, king_sq, c)) {
            add_move(move_list, king_sq, c, CASTLING_FLAG);
        }
    }
}

// -- Sliding piece moves (rooks, bishops, queens) --
void generate_sliding_moves(Board *b, MoveList *move_list, bool only_captures) {
    static const int directions[8] = {1, -1, 8, -8, 9, -9, 7, -7};
    int c = b->side_to_move ? 6 : 0;
    // rooks (idx 4), bishops (3), queens (5)
    for (int type = 3; type <= 5; type++) {
        int start_dir = (type == 4) ? 0 : (type == 3) ? 4 : 0;
        int end_dir = (type == 4) ? 4 : 8;
        U64 pieces = b->bitboards[c + type];
        while (pieces) {
            int i = bit_scan_forward(pieces);
            pieces &= pieces - 1;
            for (int d = start_dir; d < end_dir; ++d) {
                int n = i;
                while (true) {
                    int next = n + directions[d];
                    if (next < 0 || next >= 64 || abs((n % 8) - (next % 8)) > 2) break;
                    Piece target = b->squares[next];
                    if (is_friendly(b, target)) break;
                    if (only_captures && target == NONE) {
                        n = next; // skip empty squares if only captures are requested
                        continue;
                    }
                    add_move(move_list, i, next, 0);
                    if (target != NONE) break;
                    n = next;
                }
            }
        }
    }
}

// -- Knight moves --
void generate_knight_moves(Board *b, MoveList *move_list, bool only_captures) {
    U64 knights = b->bitboards[piece_to_bb_idx(KNIGHT | friendly_colour(b))];
    U64 mask = ~friendly_occupancy(b);
    if (only_captures) mask &= opponent_occupancy(b);
    while (knights) {
        int i = bit_scan_forward(knights);
        knights &= knights - 1;
        U64 targets = knight_attacks[i] & mask;
        while (targets) {
            add_move(move_list, i, bit_scan_forward(targets), 0);
            targets &= targets - 1;
        }
    }
}

// -- Pawn moves (single, double, capture, en passant, promotion) --
void generate_pawn_moves(Board *b, MoveList *move_list, bool only_captures) {
    int direction = b->side_to_move ? -8 : 8;
    int start_rank = b->side_to_move ? 6 : 1;
    int promotion_rank = b->side_to_move ? 0 : 7;
    U64 pawns = b->bitboards[piece_to_bb_idx(PAWN | friendly_colour(b))];
    while (pawns) {
        int sq_idx = bit_scan_forward(pawns);
        pawns &= pawns - 1;
        // Forward move
        int to = sq_idx + direction;
        if (!only_captures && (to >= 0 && to < 64 && b->squares[to] == NONE)) {
            if (to / 8 == promotion_rank) {
                // Add the 4 different possible promotion moves
                add_move(move_list, sq_idx, to, PROMOTION_QUEEN_FLAG); 
                add_move(move_list, sq_idx, to, PROMOTION_KNIGHT_FLAG);
                add_move(move_list, sq_idx, to, PROMOTION_ROOK_FLAG); 
                add_move(move_list, sq_idx, to, PROMOTION_BISHOP_FLAG);
            } else {
                // Else it can't promote but still check if it can double push
                add_move(move_list, sq_idx, to, 0);
                // Double push from start
                if (sq_idx / 8 == start_rank) {
                    int dbl = sq_idx + 2 * direction;
                    if (b->squares[dbl] == NONE) {
                        add_move(move_list, sq_idx, dbl, DOUBLE_PAWN_FLAG);
                    }
                }
            }
        }
        // Captures
        for (int df = -1; df <= 1; df += 2) {
            int file = sq_idx % 8;
            int cap_file = file + df;
            if (cap_file < 0 || cap_file > 7) continue;
            int cap = sq_idx + direction + df;
            if (cap < 0 || cap >= 64) continue;
            if (b->squares[cap] != NONE && is_opponent(b, b->squares[cap])) {
                if (cap / 8 == promotion_rank) {
                    add_move(move_list, sq_idx, cap, PROMOTION_QUEEN_FLAG); 
                    add_move(move_list, sq_idx, cap, PROMOTION_KNIGHT_FLAG);
                    add_move(move_list, sq_idx, cap, PROMOTION_ROOK_FLAG); 
                    add_move(move_list, sq_idx, cap, PROMOTION_BISHOP_FLAG);
                } else {
                    add_move(move_list, sq_idx, cap, 0);
                }
            }
        }
        // En passant
        if (b->enpassant_square < 64) {
            int file = sq_idx % 8;
            int ep_rank = b->enpassant_square / 8;
            // 5 hours of debugging presents you with this one if statement check :)
            if ((!b->side_to_move && ep_rank == 5) || (b->side_to_move && ep_rank == 2)) {
                for (int df = -1; df <= 1; df += 2) {
                    int cap_file = file + df;
                    if (cap_file < 0 || cap_file > 7) continue;
                    int cap = sq_idx + direction + df;
                    if (cap == b->enpassant_square) {
                        add_move(move_list, sq_idx, cap, ENPASSANT_FLAG);
                    }
                }
            }
        }
    }
}

// Check if a king is attacked after moving to a square
bool king_in_check_after_move(Board *b, int from, int to) {
    Move king_move = encode_move(from, to, 0);
    return get_check(b, king_move, 2);
}

// Check if the game is over from the current board state
bool check_end_game_conditions(Board *b, int legal_moves_count) {
    bool is_king_in_check = king_in_check(b);
    if (is_checkmate(legal_moves_count, is_king_in_check)) {
        print_board(b, true);
        std::cout << (b->side_to_move == 0 ? "White" : "Black") << " is checkmated. "
                  << (b->side_to_move == 1 ? "White" : "Black") << " wins." << std::endl;
        return false;
    }

    if (is_draw(b->half_move_clock)) {
        print_board(b, true);
        std::cout << "Draw by fifty-move rule!" << std::endl;
        return false;
    }

    if (repetition_count(b) >= 2) {
        print_board(b, true);
        std::cout << "Draw by threefold repetition!" << std::endl;
        return false;
    }

    if (is_insufficient_material(b)) {
        print_board(b, true);
        std::cout << "Draw by insufficient material!" << std::endl;
        return false;
    }

    if (is_stalemate(legal_moves_count, is_king_in_check)) {
        print_board(b, true);
        std::cout << "Stalemate!" << std::endl;
        return false;
    }

    return true;
}

bool is_checkmate(int legal_moves_count, bool is_king_in_check) {
    return (legal_moves_count == 0) && is_king_in_check;
}

bool is_draw(uint8_t half_move_clock) {
    return half_move_clock >= 100;
}

bool is_stalemate(int legal_moves_count, bool is_king_in_check) {
    return (legal_moves_count == 0) && !is_king_in_check;
}

Move gen_and_validate_all_moves(Board *b, MoveList *legal_move_list) {
    generate_moves(b, legal_move_list);
    if (!check_end_game_conditions(b, legal_move_list->count)) return NO_MOVES;

    return true; // indicates available moves
}

// Counts leaf nodes of the legal move tree
uint64_t perft(Board *b, int depth) {
    if (depth == 0) return 1;
    MoveList moves;
    generate_moves(b, &moves);
    if (depth == 1) return moves.count;
    uint64_t nodes = 0;
    for (int i = 0; i < moves.count; i++) {
        make_move_on_board(b, moves.moves[i], true);
        nodes += perft(b, depth - 1);
        unmake_move_on_board(b, moves.moves[i], true);
    }
    return nodes;
}
