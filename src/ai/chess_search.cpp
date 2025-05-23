#include <cstdlib>

#include "chess_search.h"
#include "eval.h"
#include "fen.h"
#include "movegen.h"
#include "util.h"

int search(Board *b, int depth, int alpha, int beta);
void sort_moves(Board *b, MoveList* move_list);
Move find_best_move(Board *b, MoveList *move_list, int depth);
Move pick_random_opening_move(Board *b, MoveList *move_list);

// infinities
constexpr int pos_infinity = 10000000;
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

    if (b->full_moves <= 3) {
        Move opening_move = pick_random_opening_move(b, move_list);
        if (opening_move != NO_MOVES) {
            std::cout << "Opening move: " << move_to_move_str(opening_move) << std::endl;
            return opening_move;
        }
    }

    int alpha = neg_infinity;
    int beta = pos_infinity;
    Move best_move = -1;

    // std::cout << "Searching for best move at depth " << depth << "..." << std::endl;
    for (int i = 0; i < move_list->count; ++i) {
        Move move = move_list->moves[i];
        std::string move_str = move_to_move_str(move);
        
        make_move_on_board(b, move);
        int score = -search(b, depth - 1, -beta, -alpha);
        unmake_move_on_board(b, move);

        std::cout << "Move " << move_str << " has score " << score << std::endl;
        if (score > alpha) {
            alpha = score;
            best_move = move;
        }
    }

    std::cout << "Best move found: " << move_to_move_str(best_move) << " with score " << alpha << std::endl;
    return best_move;
}

Move pick_random_opening_move(Board *b, MoveList *move_list) {
    // Determine which opening move array to use based on move number
    int move_index = b->full_moves -1 - 1; // 0-index - 1 for the first move
    std::cout << "Move index: " << move_index << std::endl;
    
    // Get the appropriate opening moves for this turn and color
    const Move *opening_moves = opening_moves_encoded[move_index][b->side_to_move];
    
    // Create a list of valid opening moves that are also legal
    std::vector<Move> valid_opening_moves;
    
    // Check each opening move to see if it's in the legal move list
    for (int i = 0; i < 10; ++i) {
        Move opening_move = opening_moves[i];
        
        // Check if this opening move is in the legal moves
        for (int j = 0; j < move_list->count; ++j) {
            if (move_list->moves[j] == opening_move) {
                valid_opening_moves.push_back(opening_move);
                break;
            }
        }
    }
    
    // If we found valid opening moves, pick one randomly
    if (!valid_opening_moves.empty()) {
        int random_index = rand() % valid_opening_moves.size();
        return valid_opening_moves[random_index];
    }
    
    // Fallback: if no opening moves are legal, select using negmax
    return NO_MOVES;
}

/**
 * @brief Searches for the best move using a negamax algorithm with alpha-beta pruning.
 * 
 * @param b The current board state.
 * @param depth The search depth.
 * @param alpha The alpha value for alpha-beta pruning.
 * @param beta The beta value for alpha-beta pruning.
 * @return int The evaluation score of the position.
 */
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
        return (king_in_check(b)) ? neg_infinity : 0; // Checkmate or stalemate
    }
    
    // Heuristically presort moves to have relatively better ones first
    sort_moves(b, &move_list);
    // std::cout << " Searching at depth " << depth << " with " << move_list.count << " moves." << std::endl;
    for (int i = 0; i < move_list.count; i++) {
        Move move = move_list.moves[i];
        std::string move_str = move_to_move_str(move);
        make_move_on_board(b, move);
        int eval = -search(b, depth - 1, -beta, -alpha);
        unmake_move_on_board(b, move);

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
