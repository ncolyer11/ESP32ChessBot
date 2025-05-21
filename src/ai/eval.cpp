#include <cmath>

#include "eval.h"
#include "board.h"
#include "piece.h" 

int evaluate(Board *b) {
    const Piece* board_squares = b->squares;
    bool side_to_move_is_black = b->side_to_move;

    int side_score[2] = {0, 0}; // [0] for White, [1] for Black

    for (int square_idx = 0; square_idx < 64; ++square_idx) {
        Piece piece = board_squares[square_idx];
        uint8_t type = piece & TYPE_MASK;
        if (type == NONE) continue;

        bool is_black = (piece & BLACK) != 0;
        int color_idx = is_black ? 1 : 0;
        // Flip square index for black's perspective for PSTs
        int table_idx = is_black ? flip_square(square_idx) : square_idx; 

        int base_value = 0;
        int positional_value = 0; // Initialize positional value

        // Use simple_base_values for material and apply positional adjustments
        switch (type) {
            case PAWN:   
                base_value = simple_base_values[PAWN];   
                positional_value = pawn_pos_table[table_idx];
                break;
            case KNIGHT: 
                base_value = simple_base_values[KNIGHT]; 
                positional_value = knight_pos_table[table_idx];
                break;
            case BISHOP: 
                base_value = simple_base_values[BISHOP]; 
                positional_value = bishop_pos_table[table_idx];
                break;
            case ROOK:   
                base_value = simple_base_values[ROOK];   
                positional_value = rook_pos_table[table_idx];
                break;
            case QUEEN:  
                base_value = simple_base_values[QUEEN];  
                positional_value = queen_pos_table[table_idx];
                break;
            case KING:
                base_value = 0; // King material value is 0
                // For simplicity, using midgame king table for now. 
                // You can re-introduce endgame king table based on game phase later.
                positional_value = mg_king_table[table_idx]; 
                break;
            default: break; 
        }

        // Add both base material value and positional value
        side_score[color_idx] += base_value + positional_value;
    }

    // Calculate material difference from the perspective of the side to move
    int player_idx = side_to_move_is_black ? 1 : 0;
    int opponent_idx = player_idx ^ 1;

    int material_difference = side_score[player_idx] - side_score[opponent_idx];

    return material_difference;
}

// The force_king_to_corner_endgame_eval function is currently not used in evaluate(),
// but its definition remains for future re-introduction if needed.
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

    int base_bonus = dist_to_center * 8 + king_distance * 4;

    return static_cast<int>(base_bonus * endgame_weight);
}