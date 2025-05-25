#include <cstdlib>
#include <algorithm>

#include "board.h"
#include "chess_search.h"
#include "eval.h"
#include "fen.h"
#include "move.h"
#include "movegen.h"
#include "util.h"

int search(Board *b, int depth_remaining, int alpha, int beta, int ply_count,
           HashHistory& game_history, HashCountMap& repetition_counts);
int get_score(Board *b, Move move, int depth_remaining, int alpha, int beta, int parent_ply,
              HashHistory& game_history, HashCountMap& repetition_counts, int id);
int search_all_captures(Board *b, int alpha, int beta);
void sort_moves(Board *b, MoveList* move_list);
Move find_best_move(Board *b, MoveList *move_list, int depth,
                    HashHistory& game_history, HashCountMap& repetition_counts);
Move pick_random_opening_move(Board *b, MoveList *move_list);
int get_score(Board *b, Move move, int depth_remaining, int alpha, int beta, int parent_ply, int id);
void DEBUG_hash_discrepancy(Board *b_start, Board *b_middle, Board *b_final, Move move, int id);
int opening_bonus(int full_move_number);


Move get_next_bot_move(Board *b, int depth) {
    MoveList move_list = { {}, 0 };
    Move chosen_move = find_best_move(b, &move_list, depth, board_pos_history, board_pos_counts);
    // check_end_game_conditions might not be strictly necessary if find_best_move handles NO_MOVES
    if (!check_end_game_conditions(b, move_list.count)) return NO_MOVES;
    
    std::string move_str = move_to_move_str(chosen_move);
    std::string colour_to_move = (b->side_to_move == 0 ? "White" : "Black");
    std::cout << "Chesp32 (" << colour_to_move << ") plays: " << move_str << std::endl;
    return chosen_move;
}

Move find_best_move(Board *b, MoveList *move_list, int depth,
                    HashHistory& game_history, HashCountMap& repetition_counts) {
    generate_moves(b, move_list);
    if (move_list->count == 0) return NO_MOVES; // Game over
    sort_moves(b, move_list);

    int alpha = -SEARCH_ALPHA_BETA_INF;
    int beta = SEARCH_ALPHA_BETA_INF;

    Move opening_move = NO_MOVES;
    int opening_move_score = -SEARCH_ALPHA_BETA_INF; 
    if (b->full_moves <= 3 + 1) { // as 1-indexed and starts from move 1
        opening_move = pick_random_opening_move(b, move_list);
        if (opening_move != NO_MOVES) {
            opening_move_score = get_score(b, opening_move, depth, alpha, beta, 0, game_history,
                                           repetition_counts, 1);
            opening_move_score += opening_bonus(b->full_moves);
        }
    }

    Move best_move = move_list->moves[0];
    if (opening_move != NO_MOVES && opening_move_score > alpha) {
        alpha = opening_move_score;
    }

    for (int i = 0; i < move_list->count; ++i) {
        Move move = move_list->moves[i];
        std::string move_str = move_to_move_str(move);
        int score = get_score(b, move, depth, alpha, beta, 0, game_history, repetition_counts, 2);

        if (score > alpha) {
            alpha = score;
            best_move = move;
        }
    }

    if (opening_move == NO_MOVES) return best_move;
    return (alpha > opening_move_score) ? best_move : opening_move;
}

/**
 * @brief Searches for the best move using a negamax algorithm with alpha-beta pruning.
 * @param b The current board state.
 * @param depth The search depth. // This is depth_remaining
 * @param alpha The alpha value for alpha-beta pruning.
 * @param beta The beta value for alpha-beta pruning.
 * @param ply_count The current ply from the root of the search.
 * @param game_history The history of board states for repetition detection.
 * @param repetition_counts The counts of each board state for repetition detection.
 * 
 * @return int The evaluation score of the position.
 */
int search(Board *b, int depth_remaining, int alpha, int beta, int ply_count,
           HashHistory& game_history, HashCountMap& repetition_counts) {
    Hash current_node_hash = hash_board_state(b);
    if (is_repetition(repetition_counts, current_node_hash)) {
        return 0; // Position already a repetition
    }
    if (depth_remaining == 0) {
        // return evaluate(b); // Base case: return evaluation score
        return search_all_captures(b, alpha, beta);
    }

    MoveList move_list;
    generate_moves(b, &move_list);
    
    if (move_list.count == 0) {
        return (king_in_check(b)) ? -(MATE_BASE_SCORE - ply_count) : 0; // Checkmate or stalemate
    } 
    
    sort_moves(b, &move_list); // Heuristically presort moves
    
    for (int i = 0; i < move_list.count; i++) {
        Move move = move_list.moves[i];
        // std::string move_str = move_to_move_str(move);

        int eval = get_score(b, move, depth_remaining, alpha, beta, ply_count + 1, game_history,
                             repetition_counts, 4);

        
        if (eval >= beta) {
            return beta; // Move is better for the opponent, prune the search
        }
        alpha = std::max(alpha, eval);
    }

    return alpha;
}

// Search all captures and checks, returning the best score
int search_all_captures(Board *b, int alpha, int beta) {
    int eval = evaluate(b);
    if (eval >= beta) {
        return beta; // Beta cut-off
    }
    alpha = std::max(alpha, eval);

    MoveList move_list;
    generate_moves(b, &move_list, true);
    sort_moves(b, &move_list);

    for (int i = 0; i < move_list.count; i++) {
        std::string move_str = move_to_move_str(move_list.moves[i]);
        make_move_on_board(b, move_list.moves[i], true);
        eval = -search_all_captures(b, -beta, -alpha);
        unmake_move_on_board(b, move_list.moves[i], true);

        if (eval >= beta) {
            return beta; // Beta cut-off
        }
        alpha = std::max(alpha, eval);
    }

    return alpha;
}

int get_score(Board *b, Move move, int depth_remaining, int alpha, int beta, int parent_ply,
              HashHistory& game_history, HashCountMap& repetition_counts, int id) {
    Hash hash_before = hash_board_state(b);
    Board b_copy = *b;
    make_move_on_board(b, move, true);
    Board b_move = *b;
    Hash current_hash = hash_board_state(b, true);
    add_position_hash(game_history, repetition_counts, current_hash);

    int score;
    if (is_repetition(repetition_counts, hash_board_state(b, true))) {
        std::cout << "Repetition detected for move: " << move_to_move_str(move) << std::endl;
        score = 0; // Draw by repetition
    } else {
        score = -search(b, depth_remaining - 1, -beta, -alpha, parent_ply + 1,
                        game_history, repetition_counts); // Pass history/counts down
    }
    remove_last_position_hash(game_history, repetition_counts);
    unmake_move_on_board(b, move, true);
    Hash hash_after = hash_board_state(b);

    if (hash_before != hash_after) DEBUG_hash_discrepancy(&b_copy, &b_move, b, move, id);

    return score;
}

bool get_check(Board *b, Move move, int id) { 
    Hash hash_before = hash_board_state(b);
    Board b_copy = *b;
    make_move_on_board(b, move, true);
    Board b_move = *b;
    b->side_to_move ^= 1;
    bool in_check = king_in_check(b);
    b->side_to_move ^= 1;
    unmake_move_on_board(b, move, true);
    Hash hash_after = hash_board_state(b);

    if (hash_before != hash_after) DEBUG_hash_discrepancy(&b_copy, &b_move, b, move, id);

    return in_check;
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
            est_move_score = 8 * simple_base_values[capture_piece_type] 
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

        move_scores[i] = est_move_score;
    }

    // Sort the moves based on their scores
    quicksort_moves(move_list->moves, move_scores, 0, move_list->count - 1);
}

Move random_search(Move *legal_move_list, int legal_move_count) {
    int random = std::rand();
    int idx = random % legal_move_count;
    Move chosen_move = legal_move_list[idx];
    return chosen_move;
}

Move pick_random_opening_move(Board *b, MoveList *move_list) {
    // Determine which opening move array to use based on move number
    int move_index = b->full_moves - 1 - 1; // 1-indexed, -1 for the first move
    
    // Get the appropriate opening moves for this turn and color
    const Move *opening_moves = opening_moves_encoded[move_index][b->side_to_move];
    
    // Create a list of valid opening moves that are also legal
    std::vector<Move> valid_opening_moves;
    
    // Check each opening move to see if it's in the legal move list
    for (int i = 0; i < 10; i++) { 
        Move opening_move = opening_moves[i];
        
        // Check if this opening move is in the legal moves
        for (int j = 0; j < move_list->count; j++) {
            if (move_list->moves[j] == opening_move) {
                valid_opening_moves.push_back(opening_move);
                break;
            }
        }
    }
    
    if (!valid_opening_moves.empty()) {
        int random_index = rand() % valid_opening_moves.size();
        return valid_opening_moves[random_index];
    }
    
    return NO_MOVES;
}

int opening_bonus(int full_move_number) {
    int move_index = full_move_number - 2;
    if (move_index >= 5) return 0;
    int bonus = 150 - 20 * move_index;
    return bonus;
}

void DEBUG_hash_discrepancy(Board *b_start, Board *b_middle, Board *b_final, Move move, int id) {
    std::cout << id << " CHECK CHECK CHECK CHECK CHECK CHECK CHECK CHECK CHECK" << std::endl;
    std::cout << "ERROR: Board state changed after make/unmake operations!" << std::endl;
    std::cout << "move: " << move_to_move_str(move) << std::endl;
    
    std::cout << "######################################" << std::endl;
    std::cout << "######################################" << std::endl;
    std::cout << "Board before make_move:" << move_to_move_str(move) << std::endl;
    print_board(b_start, true, true);
    std::cout << "Board in the middle:" << std::endl;
    print_board(b_middle, true, true);
    std::cout << "Board after make_move:" << std::endl;
    print_board(b_final, true, true);
    
    std::cout << "######################################" << std::endl;
    std::cout << "PRINTING FROM BBS" << std::endl;
    std::cout << "######################################" << std::endl;
    std::cout << "Before make_move:" << move_to_move_str(move) << std::endl;
    print_board_from_bbs(b_start, true);
    std::cout << "In the middle:" << std::endl;
    print_board_from_bbs(b_middle, true);
    std::cout << "After make_move:" << std::endl;
    print_board_from_bbs(b_final, true);
    std::cout << "######################################" << std::endl;
    std::cout << "CHECK CHECK CHECK CHECK CHECK CHECK CHECK CHECK CHECK" << std::endl;
}