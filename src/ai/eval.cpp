#include <cmath>

#include "eval.h"
#include "board.h"


int force_king_to_corner_endgame_eval(int friendly_king_square,
                                      int opponent_king_square,
                                      float endgame_weight);


int evaluate(Board *b) {
    const Piece* board_squares = b->squares;
    bool side_to_move_is_black = b->side_to_move;

    int bishop_count[2] = {0, 0};
    int rook_count[2] = {0, 0};
    int queen_count[2] = {0, 0};
    int king_square_idx[2] = {0, 0};

    for (int square_idx = 0; square_idx < 64; ++square_idx) {
        Piece piece = board_squares[square_idx];
        uint8_t type = piece & TYPE_MASK;
        if (type == NONE) continue;

        bool is_black = (piece & BLACK) != 0;
        int color_idx = is_black ? 1 : 0;
        if (type == BISHOP) {
            bishop_count[color_idx]++;
        } else if (type == ROOK) {
            rook_count[color_idx]++;
        } else if (type == QUEEN) {
            queen_count[color_idx]++;
        } else if (type == KING) {
            king_square_idx[color_idx] = square_idx;
        }
    }

    bool two_queens = (queen_count[0] > 0 && queen_count[1] > 0);
    bool no_queens = (queen_count[0] == 0 && queen_count[1] == 0);
    const PhaseValues* phase_values;
    if (two_queens) {
        phase_values = &midgame_values;
    } else if (no_queens) {
        phase_values = &endgame_values;
    } else {
        phase_values = &threshold_values;
    }

    int side_score[2]        = {0, 0};
    int processed_rooks[2]   = {0, 0};
    int processed_queens[2]  = {0, 0};

    for (int square_idx = 0; square_idx < 64; ++square_idx) {
        Piece piece       = board_squares[square_idx];
        uint8_t type      = piece & TYPE_MASK;
        if (type == NONE) continue;
        bool is_black     = (piece & BLACK) != 0;
        int color_idx   = is_black ? 1 : 0;
        int table_idx   = is_black ? flip_square(square_idx) : square_idx;

        int positional_value = 0;
        if (type == KING) {
            positional_value = no_queens ? eg_king_table[table_idx] : mg_king_table[table_idx];
        } else {
            switch (type) {
                case PAWN: positional_value = pawn_pos_table[table_idx]; break;
                case KNIGHT: positional_value = knight_pos_table[table_idx]; break;
                case BISHOP: positional_value = bishop_pos_table[table_idx]; break;
                case ROOK: positional_value = rook_pos_table[table_idx]; break;
                case QUEEN: positional_value = queen_pos_table[table_idx]; break;
            }
        }

        int base_value = 0;
        switch (type) {
            case PAWN:
                base_value = phase_values->pawn;
                break;
            case KNIGHT:
                base_value = phase_values->knight;
                break;
            case BISHOP:
                base_value = phase_values->bishop;
                break;
            case ROOK: {
                int rook_idx = processed_rooks[color_idx] < 2
                    ? processed_rooks[color_idx]
                    : 1;
                base_value = phase_values->rook[rook_idx];
                ++processed_rooks[color_idx];
                break;
            }
            case QUEEN: {
                int queen_idx = processed_queens[color_idx] < 2
                    ? processed_queens[color_idx]
                    : 1;
                base_value = phase_values->queen[queen_idx];
                ++processed_queens[color_idx];
                break;
            }
            case KING:
                base_value = 0;
                break;
            default:
                break;
        }

        side_score[color_idx] += base_value + positional_value;
    }

    for (int color_idx = 0; color_idx < 2; ++color_idx) {
        if (bishop_count[color_idx] >= 2) {
            side_score[color_idx] += phase_values->bishop_pair_bonus;
        }
    }

    int player_idx = side_to_move_is_black ? 1 : 0;
    int opponent_idx = player_idx ^ 1;
    int material_difference = side_score[player_idx] - side_score[opponent_idx];
    float corner_weight = no_queens ? 1.0f : 0.0f;
    int corner_bonus = force_king_to_corner_endgame_eval(
        king_square_idx[player_idx],
        king_square_idx[opponent_idx],
        corner_weight
    );

    return material_difference + corner_bonus;
}

int force_king_to_corner_endgame_eval(int friendly_king_square,
                                      int opponent_king_square,
                                      float endgame_weight) {
    int opp_file = opponent_king_square & 7;
    int opp_rank = opponent_king_square >> 3;
    int friend_file = friendly_king_square & 7;
    int friend_rank = friendly_king_square >> 3;

    int dist_to_center_file = opp_file < 4 ? (3 - opp_file) : (opp_file - 4);
    int dist_to_center_rank = opp_rank < 4 ? (3 - opp_rank) : (opp_rank - 4);
    int dist_to_center = dist_to_center_file + dist_to_center_rank;

    int king_file_diff = std::abs(opp_file - friend_file);
    int king_rank_diff = std::abs(opp_rank - friend_rank);
    int king_distance = king_file_diff + king_rank_diff;

    // combine opponent’s distance from center and separation from friendly king
    int base_bonus = dist_to_center * 8 + king_distance * 4;

    return static_cast<int>(base_bonus * endgame_weight);
}

