#include <cmath>

#include "eval.h"
#include "board.h"
#include "piece.h" 

int manhattan_distance(int square1, int square2);
int num_rook_moves_to_reach_square(int from, int to);
int mop_up_eval(
    Board *b,
    int my_material,
    int opponent_material,
    float endgame_weight
);

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
        // Flip square index for white's perspective for PSTs as white is on the bottom
        int table_idx = is_black ? square_idx : flip_square(square_idx); 

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
                base_value = simple_base_values[KING];
                positional_value = mg_king_table[table_idx]; 
                break;
        }

        // Add both base material value and positional value
        side_score[color_idx] += base_value + positional_value;

        // Debug: Print piece info and score
        // std::cout << "Square: " << square_idx << ", Piece: " << (int)type
        //           << ", Color: " << (is_black ? "Black" : "White")
        //           << ", Base: " << base_value
        //           << ", Positional: " << positional_value
        //           << ", SideScore[" << color_idx << "]: " << side_score[color_idx] << std::endl;
    }

    // Calculate material difference from the perspective of the side to move
    int player_idx = side_to_move_is_black ? 1 : 0;
    int opponent_idx = player_idx ^ 1;

    int material_difference = side_score[player_idx] - side_score[opponent_idx];

    // Debug: Print final scores
    // std::cout << "Final Side Scores: White=" << side_score[0]
    //           << ", Black=" << side_score[1]
    //           << ", Side to move: " << (side_to_move_is_black ? "Black" : "White")
    //           << ", Eval: " << material_difference << std::endl;

    int my_material = 0;
    int opponent_material = 0;
    float endgame_weight = 0.0f;
    int mop_up_score = mop_up_eval(b, my_material, opponent_material, endgame_weight);

    return material_difference + mop_up_score;
}

// The force_king_to_corner_endgame_eval function is currently not used in evaluate(),
// but its definition remains for future re-introduction if needed.
int mop_up_eval(
    Board *b,
    int my_material,
    int opponent_material,
    float endgame_weight
) {
    int mop_up_score = 0;
    int friendly_colour = b->side_to_move ? BLACK : WHITE;
    int opponent_colour = b->side_to_move ? WHITE : BLACK;
    int pawn_threshold = simple_base_values[2] * 2;
    if (my_material > opponent_material + pawn_threshold && endgame_weight > 0.0f) {

        int friendly_king_square = bit_scan_forward(b->bitboards[KING | friendly_colour]);
        int opponent_king_square = bit_scan_forward(b->bitboards[KING | opponent_colour]);
        mop_up_score += manhattan_distance(friendly_king_square, opponent_king_square) * 10;
        int ortho_dist = num_rook_moves_to_reach_square(friendly_king_square, opponent_king_square);
        mop_up_score += (14 - ortho_dist) * 4;
        return static_cast<int>(mop_up_score * endgame_weight);
    }
    return 0;
}

int manhattan_distance(int square1, int square2) {
    int file1 = square1 % 8;
    int rank1 = square1 / 8;
    int file2 = square2 % 8;
    int rank2 = square2 / 8;
    return std::abs(file1 - file2) + std::abs(rank1 - rank2);
}

int num_rook_moves_to_reach_square(int from, int to) {
    int file_diff = std::abs((from % 8) - (to % 8));
    int rank_diff = std::abs((from / 8) - (to / 8));
    return file_diff + rank_diff;
}
