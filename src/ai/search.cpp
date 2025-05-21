#include <cstdlib>

#include "eval.h"
#include "movegen.h"
#include "search.h"

int search(int depth, int alpha, int beta);
void sort_moves(MoveList* move_list);
Move find_best_move(int depth);

// infinities
constexpr int pos_infinity =  10000000;
constexpr int neg_infinity = -pos_infinity;

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
Move get_next_bot_move() {
    Move chosen_move = find_best_move(3);
    std::string move_str = move_to_move_str(chosen_move);
    std::string colour_to_move = (board.side_to_move == 0 ? "White" : "Black");
    std::cout << "Chesp32 (" << colour_to_move << ") plays: " << move_str << std::endl;
    return chosen_move;
}

// Assumes legal_move_list is an array or vector of Move, and legal_move_count is its size
Move random_search(const Move* legal_move_list, int legal_move_count) {
    int random = std::rand();
    int idx = random % legal_move_count;
    Move chosen_move = legal_move_list[idx];
    return chosen_move;
}

Move find_best_move(int depth) {
    MoveList move_list = generate_moves_arr();
    std::cout << "Moves before sorting:" << std::endl;
    for (int i = 0; i < move_list.count; ++i) {
        std::cout << move_to_move_str(move_list.moves[i]) << " ";
    }
    std::cout << std::endl;
    sort_moves(&move_list);
    std::cout << "Moves after sorting:" << std::endl;
    for (int i = 0; i < move_list.count; ++i) {
        std::cout << move_to_move_str(move_list.moves[i]) << " ";
    }
    std::cout << std::endl;

    int alpha = neg_infinity;
    int beta = pos_infinity;
    Move best_move = -1;

    std::cout << "Searching for best move at depth " << depth << "..." << std::endl;
    for (int i = 0; i < move_list.count; ++i) {
        Move move = move_list.moves[i];
        std::string move_str = move_to_move_str(move);
        std::cout << "Evaluating move: " << move_str << std::endl;

        make_move_on_board(&board, move);
        int score = -search(depth - 1, -beta, -alpha);
        unmake_move_on_board(&board, move);

        std::cout << "Move " << move_str << " has score " << score << std::endl;

        if (score > alpha) {
            alpha = score;
            best_move = move;
            std::cout << "New best move: " << move_str << " with score " << score << std::endl;
        }
    }

    std::cout << "Best move found: " << move_to_move_str(best_move) << " with score " << alpha << std::endl;
    return best_move;
}

int search(int depth, int alpha, int beta) {
    std::cout << "search(depth=" << depth << "), side to move: " 
              << board.side_to_move << ", alpha: " << alpha << ", beta: " << beta << std::endl;
    if (depth == 0) {
        return evaluate();
    }

    MoveList move_list = generate_moves_arr();
    std::cout << "Move count: " << move_list.count << std::endl;

    if (move_list.count == 0) {
        std::cout << "No legal moves available." << std::endl;
        if (king_in_check(&board, board.side_to_move)) {
            return neg_infinity; // Checkmate
        }
        return 0; // Stalemate
    }

    // Heuristically presort moves to have relatively better ones first
    sort_moves(&move_list);
    for (int i = 0; i < move_list.count; ++i) {
        Move move = move_list.moves[i];
        make_move_on_board(&board, move);
        int eval = -search(depth - 1, -beta, -alpha);
        unmake_move_on_board(&board, move);
        if (eval >= beta) {
            // Move is better for the opponent, prune the search
            return beta;
        }
        alpha = std::max(alpha, eval);
    }

    return alpha;
}

// Prioritise captures and checks
void sort_moves(MoveList* move_list) {
    int move_scores[move_list->count];
    for (int i = 0; i < move_list->count; i++) {
        Move move = move_list->moves[i];
        int est_move_score = 0;
        Piece move_piece_type = board.squares[move_from(move)] & TYPE_MASK;
        Piece capture_piece_type = board.squares[move_to(move)] & TYPE_MASK;

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
    std::cout << "Move scores: ";
    for (int i = 0; i < move_list->count; i++) {
        std::cout << move_scores[i] << " ";
    }
    // Sort the moves based on their scores using quick sort
    quicksort_moves(move_list->moves, move_scores, 0, move_list->count - 1);
}
