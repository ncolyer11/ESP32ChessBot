#include <cstdlib>

#include "chess_search.h"
#include "eval.h"
#include "fen.h"
#include "movegen.h"
#include "util.h"

int search(Board *b, int depth, int alpha, int beta);
void sort_moves(Board *b, MoveList* move_list);
Move find_best_move(Board *b, MoveList *move_list, int depth);

// infinities
constexpr int pos_infinity = 10000000;
constexpr int neg_infinity = -pos_infinity;

// Store the last 5 sets of 12 bitboards for debugging or analysis
U64 bitboard_history[BITBOARD_HISTORY_SIZE][2] = {0};
Board b_history[BITBOARD_HISTORY_SIZE] = {};
int bitboard_history_index = 0;

void add_to_bitboard_history(Board *b, Board b_history[BITBOARD_HISTORY_SIZE], U64 white_pawns, 
                             U64 black_pawns) {
    bitboard_history[bitboard_history_index][0] = white_pawns;
    bitboard_history[bitboard_history_index][1] = black_pawns;
    b_history[bitboard_history_index] = *b;
    bitboard_history_index = (bitboard_history_index + 1) % BITBOARD_HISTORY_SIZE;
}

/**
 * @brief Generates all legal moves for the current board position (using the global board), 
 * picks a random move from the legal move list, and returns it in algebraic notation.
 * 
 * Globals Used:
 *   - board (input): The current board state, including side_to_move.
 *   - legal_move_list, legal_move_count (input): Populated by generate_moves() and filter_legal_moves().
 *
 * Returns:
 *   - std::string: The chosen move in algebraic notation (e.g., "e2e4" or "e7e8q" for promotion).
 *
 * Side Effects:
 *   - Prints the chosen move to stdout.
 *
 * Usage:
 *   Call after generate_moves() and filter_legal_moves() to pick a random move.
 */
Move get_next_bot_move(Board *b, int depth) {
    MoveList move_list = { {}, 0 };
    Move chosen_move = find_best_move(b, &move_list, depth);
    if (!check_end_game_conditions(b, move_list.count)) return NO_MOVES;
    
    std::string move_str = move_to_move_str(chosen_move);
    std::string colour_to_move = (b->side_to_move == 0 ? "White" : "Black");
    std::cout << "Chesp32 (" << colour_to_move << ") plays: " << move_str << std::endl;
    return chosen_move;
}

Move find_best_move(Board *b, MoveList *move_list, int depth) {
    generate_moves(b, move_list);
    sort_moves(b, move_list);

    int alpha = neg_infinity;
    int beta = pos_infinity;
    Move best_move = -1;

    // std::cout << "Searching for best move at depth " << depth << "..." << std::endl;
    for (int i = 0; i < move_list->count; ++i) {
        Move move = move_list->moves[i];
        std::string move_str = move_to_move_str(move);
        // print_board(b, true, true);
        add_to_bitboard_history(b, b_history, b->bitboards[1], b->bitboards[7]);
        exit_if_corrupt_bits(b, b_history, bitboard_history, bitboard_history_index);
        
        make_move_on_board(b, move);
        add_to_bitboard_history(b, b_history, b->bitboards[1], b->bitboards[7]);
        // print_board(b, true, true);
        exit_if_corrupt_bits(b, b_history, bitboard_history, bitboard_history_index);

        int score = -search(b, depth - 1, -beta, -alpha);
        // print_board(b, true, true);
        exit_if_corrupt_bits(b, b_history, bitboard_history, bitboard_history_index);

        unmake_move_on_board(b, move);
        // print_board(b, true, true);
        exit_if_corrupt_bits(b, b_history, bitboard_history, bitboard_history_index);

        // std::cout << "Move " << move_str << " has score " << score << std::endl;
        if (score > alpha) {
            alpha = score;
            best_move = move;
        }
    }

    // std::cout << "Best move found: " << move_to_move_str(best_move) << " with score " << alpha << std::endl;
    return best_move;
}

int search(Board *b, int depth, int alpha, int beta) {
    if (depth == 0) {
        return evaluate(b);
    }

    MoveList move_list;
    generate_moves(b, &move_list);
    // std::cout << "Move count: " << move_list.count << std::endl;
    // std::cout << "Moves (d, s, a, b: " << depth << ", " << b->side_to_move << ", " 
    //           << alpha << ", " << beta << "): ";
    
    if (move_list.count == 0) {
        if (king_in_check(b)) {
            return neg_infinity; // Checkmate
        }
        return 0; // Stalemate
    }
    
    // Heuristically presort moves to have relatively better ones first
    sort_moves(b, &move_list);
    // std::cout << " Searching at depth " << depth << " with " << move_list.count << " moves." << std::endl;
    for (int i = 0; i < move_list.count; ++i) {
        Move move = move_list.moves[i];
        std::string move_str = move_to_move_str(move);
        // print_board(b, true, true);
        add_to_bitboard_history(b, b_history, b->bitboards[1], b->bitboards[7]);
        exit_if_corrupt_bits(b, b_history, bitboard_history, bitboard_history_index);
        
        make_move_on_board(b, move);
        add_to_bitboard_history(b, b_history, b->bitboards[1], b->bitboards[7]);
        // print_board(b, true, true);
        exit_if_corrupt_bits(b, b_history, bitboard_history, bitboard_history_index);

        int eval = -search(b, depth - 1, -beta, -alpha);
        // print_board(b, true, true);
        exit_if_corrupt_bits(b, b_history, bitboard_history, bitboard_history_index);

        unmake_move_on_board(b, move);
        // print_board(b, true, true);
        exit_if_corrupt_bits(b, b_history, bitboard_history, bitboard_history_index);

        if (eval >= beta) {
            return beta; // Move is better for the opponent, prune the search
        }
        alpha = std::max(alpha, eval);
    }

    return alpha;
}

// Prioritise captures and checks
void sort_moves(Board *b, MoveList* move_list) {
    int move_scores[move_list->count];
    for (int i = 0; i < move_list->count; i++) {
        Move move = move_list->moves[i];
        int est_move_score = 0;
        Piece move_piece_type = b->squares[move_from(move)] & TYPE_MASK;
        Piece capture_piece_type = b->squares[move_to(move)] & TYPE_MASK;

        // Good value trades are preferred
        if (capture_piece_type != NONE) {
            est_move_score = 9 * simple_base_values[capture_piece_type] 
                               - simple_base_values[move_piece_type];
        }

        // As well as promotions
        switch (move_flag(move)) {
            case PROMOTION_QUEEN_FLAG:   est_move_score += simple_base_values[QUEEN];   break;
            case PROMOTION_KNIGHT_FLAG:  est_move_score += simple_base_values[KNIGHT];  break;
            case PROMOTION_ROOK_FLAG:    est_move_score += simple_base_values[ROOK];    break;
            case PROMOTION_BISHOP_FLAG:  est_move_score += simple_base_values[BISHOP];  break;
        }

        // And moving a piece to where a pawn can attack it is also a bad idea
        // ... todo

        // Store the score
        move_scores[i] = est_move_score;
    }
    // Sort the moves based on their scores using quick sort
    quicksort_moves(move_list->moves, move_scores, 0, move_list->count - 1);
}

Move random_search(Move *legal_move_list, int legal_move_count) {
    int random = std::rand();
    int idx = random % legal_move_count;
    Move chosen_move = legal_move_list[idx];
    return chosen_move;
}
