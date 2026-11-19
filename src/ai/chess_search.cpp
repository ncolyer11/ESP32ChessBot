#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>

#include "board.h"
#include "chess_search.h"
#include "eval.h"
#include "fen.h"
#include "move.h"
#include "movegen.h"
#include "util.h"

// ---------------------------------------------------------------------------
// Tunables / constants
// ---------------------------------------------------------------------------
constexpr int MAX_PLY = 64;                       // search stack bound (fixed arrays)
constexpr int MAX_DEPTH = 48;
constexpr int MAX_QUIESCENCE_PLY = 8;             // cap on extra capture plies
constexpr int MATE_THRESHOLD = MATE_BASE_SCORE - 1000;
constexpr int BOOK_MARGIN = 30;                   // book move accepted if within this of the best score
constexpr int TIME_CHECK_MASK = 2047;             // check the clock every 2048 nodes

#ifdef ESP_PLATFORM
constexpr int TT_BITS = 12;
#else
constexpr int TT_BITS = 16;
#endif
constexpr uint32_t TT_SIZE = 1u << TT_BITS;

enum TTBound : uint8_t { TT_NONE = 0, TT_EXACT, TT_LOWER, TT_UPPER };

struct TTEntry {
    uint32_t key;   // upper 32 bits of the zobrist hash
    int32_t score;
    uint16_t move;
    int8_t depth;
    uint8_t bound;
};

static TTEntry tt_table[TT_SIZE];

struct SearchState {
    uint64_t nodes;
    bool stopped;
    bool can_stop;  // never abort the first iteration so we always have a move
    bool timed;
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point deadline;
    Move killers[MAX_PLY][2];
};

static SearchState ss;

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
static int search(Board *b, int depth, int alpha, int beta, int ply);
static int search_all_captures(Board *b, int alpha, int beta, int q_ply);
static Move find_best_move(Board *b, MoveList *move_list, int depth, int time_ms);
static Move pick_random_opening_move(Board *b, const MoveList *move_list);
static void score_moves(Board *b, const MoveList *list, int *scores, Move tt_move, int ply);
static void pick_next_move(MoveList *list, int *scores, int start);

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static inline int score_to_tt(int score, int ply) {
    if (score >= MATE_THRESHOLD) return score + ply;
    if (score <= -MATE_THRESHOLD) return score - ply;
    return score;
}

static inline int score_from_tt(int score, int ply) {
    if (score >= MATE_THRESHOLD) return score - ply;
    if (score <= -MATE_THRESHOLD) return score + ply;
    return score;
}

static inline double elapsed_ms() {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - ss.start).count();
}

static inline void check_time() {
    if (ss.timed && ss.can_stop && (ss.nodes & TIME_CHECK_MASK) == 0 &&
        std::chrono::steady_clock::now() >= ss.deadline) {
        ss.stopped = true;
    }
}

static inline bool is_quiet(Board *b, Move m) {
    int flag = move_flag(m);
    if (flag == ENPASSANT_FLAG) return false;
    if (flag >= PROMOTION_QUEEN_FLAG && flag <= PROMOTION_BISHOP_FLAG) return false;
    return (b->squares[move_to(m)] & TYPE_MASK) == NONE;
}

static inline bool is_draw_node(Board *b) {
    return repetition_count(b) >= 1 || is_insufficient_material(b);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
Move get_next_bot_move(Board *b, int depth, int time_ms) {
    MoveList move_list = { {}, 0 };
    Move chosen_move = find_best_move(b, &move_list, depth, time_ms);
    // check_end_game_conditions might not be strictly necessary if find_best_move handles NO_MOVES
    if (!check_end_game_conditions(b, move_list.count)) return NO_MOVES;

    std::string colour_to_move = (b->side_to_move == 0 ? "White" : "Black");
    std::cout << "Chesp32 (" << colour_to_move << ") plays: " << move_to_move_str(chosen_move) << std::endl;
    return chosen_move;
}

// Root search with iterative deepening, previous best move searched first
static Move find_best_move(Board *b, MoveList *move_list, int max_depth, int time_ms) {
    generate_moves(b, move_list);
    if (move_list->count == 0) return NO_MOVES; // Game over

    max_depth = std::max(1, std::min(max_depth, MAX_DEPTH));

    ss.nodes = 0;
    ss.stopped = false;
    ss.can_stop = false;
    ss.timed = time_ms > 0;
    ss.start = std::chrono::steady_clock::now();
    ss.deadline = ss.start + std::chrono::milliseconds(time_ms);
    for (int i = 0; i < MAX_PLY; i++) ss.killers[i][0] = ss.killers[i][1] = NO_MOVES;

    // initial root ordering
    int scores[MAX_MOVES];
    score_moves(b, move_list, scores, NO_MOVES, 0);
    for (int i = 0; i < move_list->count; i++) pick_next_move(move_list, scores, i);

    Move best_move = move_list->moves[0];
    int best_score = 0;
    int depth_reached = 0;

    for (int depth = 1; depth <= max_depth; depth++) {
        // Next iteration costs a few times the last one, so don't start it with less than half the time left
        if (depth > 1 && ss.timed && elapsed_ms() > time_ms * 0.5) break;
        ss.can_stop = depth > 1;

        int alpha = -SEARCH_ALPHA_BETA_INF;
        int beta = SEARCH_ALPHA_BETA_INF;
        Move iter_best = move_list->moves[0];
        int iter_score = -SEARCH_ALPHA_BETA_INF;

        for (int i = 0; i < move_list->count; i++) {
            Move move = move_list->moves[i];
            make_move_on_board(b, move, true);
            int score;
            if (i == 0) {
                score = -search(b, depth - 1, -beta, -alpha, 1);
            } else {
                score = -search(b, depth - 1, -alpha - 1, -alpha, 1);
                if (score > alpha && score < beta && !ss.stopped) {
                    score = -search(b, depth - 1, -beta, -alpha, 1);
                }
            }
            unmake_move_on_board(b, move, true);
            if (ss.stopped) break;

            if (score > iter_score) {
                iter_score = score;
                iter_best = move;
                if (score > alpha) alpha = score;
            }
        }

        if (ss.stopped) break; // discard the unfinished iteration

        best_move = iter_best;
        best_score = iter_score;
        depth_reached = depth;

        // keep the best move at the front for the next iteration
        int idx = 0;
        while (move_list->moves[idx] != best_move) idx++;
        for (; idx > 0; idx--) std::swap(move_list->moves[idx], move_list->moves[idx - 1]);

        if (best_score >= MATE_THRESHOLD) break; // shortest mate found, nothing more to gain
    }

    // Opening book: play a book move if it is nearly as good as the best move
    if (!ss.stopped && depth_reached > 0 && std::abs(best_score) < MATE_THRESHOLD) {
        Move book_move = pick_random_opening_move(b, move_list);
        if (book_move != NO_MOVES && book_move != best_move) {
            int threshold = best_score - BOOK_MARGIN;
            make_move_on_board(b, book_move, true);
            int book_score = -search(b, depth_reached - 1, -threshold, -(threshold - 1), 1);
            unmake_move_on_board(b, book_move, true);
            if (!ss.stopped && book_score >= threshold) best_move = book_move;
        }
    }

    if (!MODE) {
        std::cout << "info depth " << depth_reached << " nodes " << ss.nodes << " time "
                  << static_cast<int>(elapsed_ms()) << "ms score ";
        if (std::abs(best_score) >= MATE_THRESHOLD) {
            int mate_plies = MATE_BASE_SCORE - std::abs(best_score);
            std::cout << "mate " << (best_score > 0 ? "" : "-") << (mate_plies + 1) / 2;
        } else {
            std::cout << "cp " << best_score;
        }
        std::cout << std::endl;
    }

    return best_move;
}

/**
 * @brief Negamax with alpha-beta, PVS, transposition table and killer moves.
 * @param depth Remaining depth.
 * @param ply Distance from the root (mate score = MATE_BASE_SCORE - ply).
 * @return Score from the side to move's perspective.
 */
static int search(Board *b, int depth, int alpha, int beta, int ply) {
    if (is_draw_node(b)) return 0; // repetition or dead material

    // 50 move rule, but checkmate on the 100th half move still counts as mate
    if (b->half_move_clock >= 100) {
        if (king_in_check(b)) {
            MoveList replies;
            generate_moves(b, &replies);
            if (replies.count == 0) return -(MATE_BASE_SCORE - ply);
        }
        return 0;
    }

    if (depth <= 0 || ply >= MAX_PLY - 1) return search_all_captures(b, alpha, beta, 0);

    ss.nodes++;
    check_time();
    if (ss.stopped) return 0;

    const bool is_pv = beta - alpha > 1;
    const int alpha_orig = alpha;

    // Transposition table probe
    TTEntry *entry = &tt_table[b->zobrist & (TT_SIZE - 1)];
    const uint32_t key_check = static_cast<uint32_t>(b->zobrist >> 32);
    Move tt_move = NO_MOVES;
    if (entry->bound != TT_NONE && entry->key == key_check) {
        tt_move = entry->move;
        if (!is_pv && entry->depth >= depth) {
            int s = score_from_tt(entry->score, ply);
            if (entry->bound == TT_EXACT) return s;
            if (entry->bound == TT_LOWER && s >= beta) return s;
            if (entry->bound == TT_UPPER && s <= alpha) return s;
        }
    }

    MoveList move_list;
    generate_moves(b, &move_list);

    if (move_list.count == 0) {
        return king_in_check(b) ? -(MATE_BASE_SCORE - ply) : 0; // Checkmate or stalemate
    }

    int scores[MAX_MOVES];
    score_moves(b, &move_list, scores, tt_move, ply);

    int best = -SEARCH_ALPHA_BETA_INF;
    Move best_move = NO_MOVES;

    for (int i = 0; i < move_list.count; i++) {
        pick_next_move(&move_list, scores, i);
        Move move = move_list.moves[i];
        const bool quiet = is_quiet(b, move);

        make_move_on_board(b, move, true);
        int score;
        if (i == 0) {
            score = -search(b, depth - 1, -beta, -alpha, ply + 1);
        } else {
            score = -search(b, depth - 1, -alpha - 1, -alpha, ply + 1);
            if (score > alpha && score < beta && !ss.stopped) {
                score = -search(b, depth - 1, -beta, -alpha, ply + 1);
            }
        }
        unmake_move_on_board(b, move, true);
        if (ss.stopped) return 0;

        if (score > best) {
            best = score;
            best_move = move;
            if (score > alpha) alpha = score;
            if (alpha >= beta) {
                if (quiet && ss.killers[ply][0] != move) {
                    ss.killers[ply][1] = ss.killers[ply][0];
                    ss.killers[ply][0] = move;
                }
                break;
            }
        }
    }

    // Store in the TT (always replace, unless it's the same position searched deeper)
    if (entry->bound == TT_NONE || entry->key != key_check || entry->depth <= depth) {
        entry->key = key_check;
        entry->score = score_to_tt(best, ply);
        entry->move = best_move;
        entry->depth = static_cast<int8_t>(depth);
        entry->bound = (best <= alpha_orig) ? TT_UPPER : (best >= beta ? TT_LOWER : TT_EXACT);
    }

    return best;
}

// Quiescence: only captures, with a cap on how deep it can go
static int search_all_captures(Board *b, int alpha, int beta, int q_ply) {
    ss.nodes++;
    check_time();
    if (ss.stopped) return 0;

    int eval = evaluate(b);
    if (q_ply >= MAX_QUIESCENCE_PLY) return eval;
    if (eval >= beta) return eval;
    alpha = std::max(alpha, eval);

    MoveList move_list;
    generate_moves(b, &move_list, true);

    int scores[MAX_MOVES];
    score_moves(b, &move_list, scores, NO_MOVES, -1);

    for (int i = 0; i < move_list.count; i++) {
        pick_next_move(&move_list, scores, i);
        Move move = move_list.moves[i];
        make_move_on_board(b, move, true);
        eval = -search_all_captures(b, -beta, -alpha, q_ply + 1);
        unmake_move_on_board(b, move, true);
        if (ss.stopped) return 0;

        if (eval >= beta) return eval;
        alpha = std::max(alpha, eval);
    }

    return alpha;
}

// Used by move generation to filter out moves that leave the mover's king in check
bool get_check(Board *b, Move move, int id) {
    (void)id;
    make_move_on_board(b, move, true);
    b->side_to_move ^= 1;
    bool in_check = king_in_check(b);
    b->side_to_move ^= 1;
    unmake_move_on_board(b, move, true);

    return in_check;
}

// ---------------------------------------------------------------------------
// Move ordering: TT move, captures (MVV-LVA), promotions, killers, then quiet moves
// ---------------------------------------------------------------------------
static void score_moves(Board *b, const MoveList *list, int *scores, Move tt_move, int ply) {
    const int side = b->side_to_move;
    const U64 enemy_pawns = b->bitboards[piece_to_bb_idx(PAWN | (side ? WHITE : BLACK))];

    for (int i = 0; i < list->count; i++) {
        Move move = list->moves[i];
        int from = move_from(move), to = move_to(move), flag = move_flag(move);
        Piece mover = b->squares[from] & TYPE_MASK;
        Piece victim = b->squares[to] & TYPE_MASK;
        if (flag == ENPASSANT_FLAG) victim = PAWN;

        int s = 0;
        if (move == tt_move) {
            s = 1000000;
        } else {
            if (victim != NONE) {
                // Good value trades are preferred
                s = 20000 + 8 * simple_base_values[victim] - simple_base_values[mover];
            }

            switch (flag) {
                case PROMOTION_QUEEN_FLAG:   s += simple_base_values[QUEEN];  break;
                case PROMOTION_KNIGHT_FLAG:  s += simple_base_values[KNIGHT]; break;
                case PROMOTION_ROOK_FLAG:    s += simple_base_values[ROOK];   break;
                case PROMOTION_BISHOP_FLAG:  s += simple_base_values[BISHOP]; break;
            }
            if (flag >= PROMOTION_QUEEN_FLAG && flag <= PROMOTION_BISHOP_FLAG && victim == NONE) s += 20000;

            // Moving a piece to where an enemy pawn attacks it is a bad idea
            if (mover != PAWN && mover != KING && (pawn_attacks[side][to] & enemy_pawns)) {
                s -= simple_base_values[mover];
            }

            if (victim == NONE && ply >= 0) {
                if (move == ss.killers[ply][0]) s += 9000;
                else if (move == ss.killers[ply][1]) s += 8000;
            }
        }
        scores[i] = s;
    }
}

// Selection step: bring the best remaining move to position `start`
static void pick_next_move(MoveList *list, int *scores, int start) {
    int best = start;
    for (int i = start + 1; i < list->count; i++) {
        if (scores[i] > scores[best]) best = i;
    }
    if (best != start) {
        std::swap(list->moves[start], list->moves[best]);
        std::swap(scores[start], scores[best]);
    }
}

// ---------------------------------------------------------------------------
// Opening book
// ---------------------------------------------------------------------------
static Move pick_random_opening_move(Board *b, const MoveList *move_list) {
    int move_index = b->full_moves - 1; // full_moves is 1-indexed
    if (move_index < 0 || move_index >= 5) return NO_MOVES;

    const Move *opening_moves = opening_moves_encoded[move_index][b->side_to_move];

    // Keep only the book moves that are legal right now
    Move valid_opening_moves[10];
    int valid_count = 0;
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < move_list->count; j++) {
            if (move_list->moves[j] == opening_moves[i]) {
                valid_opening_moves[valid_count++] = opening_moves[i];
                break;
            }
        }
    }

    if (valid_count > 0) return valid_opening_moves[std::rand() % valid_count];
    return NO_MOVES;
}
