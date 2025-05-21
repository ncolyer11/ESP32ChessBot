#include <cstdlib>
#include <limits>

#include "movegen.h"
#include "eval.h"
#include "search.h"

// Assumes legal_move_list is an array or vector of Move, and legal_move_count is its size
Move random_search(const Move* legal_move_list, int legal_move_count) {
    if (legal_move_count == 0) {
        // Handle no legal moves (could throw, return a sentinel, etc.)
        return Move(); // Assumes default constructor is safe
    }
    int random = std::rand();
    int idx = random % legal_move_count;
    Move chosen_move = legal_move_list[idx];
    return chosen_move;
}

int search(int depth) {
    if (depth == 0) {
        return evaluate();
    }

    MoveList move_list = generate_moves_arr();
    if (move_list.count == 0) {
        if (king_in_check(&board, board.side_to_move)) {
            return std::numeric_limits<int>::min(); // Checkmate
        }
        return 0; // Stalemate
    }

    int best_eval = std::numeric_limits<int>::min();
    for (int i = 0; i < move_list.count; ++i) {
        Move move = move_list.moves[i];
        make_move_on_board(&board, move);
        int eval = -search(depth - 1);
        best_eval = std::max(best_eval, eval);
        unmake_move_on_board(&board, move);
    }

    return best_eval;
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
Move get_next_bot_move() {
    // Generate all legal moves
    if (gen_and_validate_all_moves() == NO_MOVES) return NO_MOVES;

    // std::cout << "Available legal moves: " << legal_move_count << ::endl;
    // for (int i = 0; i < legal_move_count; ++i) {
    //     std::string move_str = move_to_move_str(legal_move_list[i]);
    //     std::cout << "Legal move: " << move_str << std::endl;
    // }

    std::string move_str = move_to_move_str(chosen_move);
    std::string colour_to_move = (board.side_to_move == 0 ? "White" : "Black");
    std::cout << "Chesp32 (" << colour_to_move << ") plays: " << move_str << std::endl;
    return chosen_move;
}