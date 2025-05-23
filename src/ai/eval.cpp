#include <cmath>
#include <iomanip>

#include "eval.h"
#include "board.h"
#include "piece.h" 

int manhattan_distance(int square1, int square2);
int num_rook_moves_to_reach_square(int from, int to);
int mop_up_eval(Board *b, int my_material, int opponent_material, float endgame_weight );
void get_materials(Board *b, int *my_material, int *opponent_material);
float endgame_phase_weight(int materials_no_pawns);
int manhattan_distance(int square1, int square2);
int num_rook_moves_to_reach_square(int from, int to);

int evaluate(Board *b) {
    const Piece* board_squares = b->squares;
    bool side_to_move_is_black = b->side_to_move;

    // Calculate material difference from the perspective of the side to move
    int player_idx = side_to_move_is_black ? 1 : 0;
    int opponent_idx = player_idx ^ 1;
    int side_score[2] = {0, 0}; // [0] for White, [1] for Black

    int my_material, opponent_material;
    get_materials(b, &my_material, &opponent_material);
    side_score[player_idx] += my_material;
    side_score[opponent_idx] += opponent_material;

    uint8_t my_colour = b->side_to_move ? BLACK : WHITE;
    uint8_t opp_colour = b->side_to_move ? WHITE : BLACK;
    int my_mat_no_pawns = my_material - popcount(b->bitboards[PAWN | my_colour]) * simple_base_values[PAWN];
    int opponent_mat_no_pawns = opponent_material - popcount(b->bitboards[PAWN | opp_colour]) * simple_base_values[PAWN];

    float my_endgame_weight = endgame_phase_weight(my_mat_no_pawns);
    float opponent_endgame_weight = endgame_phase_weight(opponent_mat_no_pawns);

    int my_mop_up = mop_up_eval(b, my_material, opponent_material, my_endgame_weight);
    int opp_mop_up = mop_up_eval(b, my_material, opponent_material, opponent_endgame_weight);
    std::cout << "My material: " << my_material
            << ", My material (no pawns): " << my_mat_no_pawns
            << ", My endgame weight: " << std::fixed << std::setprecision(3) << my_endgame_weight
            << ", My mop up: " << my_mop_up << '\n';

    std::cout << "Opponent material: " << opponent_material
            << ", Opponent material (no pawns): " << opponent_mat_no_pawns
            << ", Opponent endgame weight: " << std::fixed << std::setprecision(3) << opponent_endgame_weight
            << ", Opponent mop up: " << opp_mop_up << '\n';

    side_score[player_idx] += my_mop_up;
    side_score[opponent_idx] += opp_mop_up;

    for (int square_idx = 0; square_idx < 64; ++square_idx) {
        Piece piece = board_squares[square_idx];
        uint8_t type = piece & TYPE_MASK;
        if (type == NONE) continue;

        bool is_black = (piece & BLACK) != 0;
        int color_idx = is_black ? 1 : 0;
        // Flip square index for white's perspective for PSTs as white is on the bottom
        int table_idx = is_black ? square_idx : flip_square(square_idx); 

        int positional_value = 0; // Initialize positional value

        // Use simple_base_values for material and apply positional adjustments
        switch (type) {
            case PAWN:   positional_value = mg_pawn_pos_table[table_idx]; break;
            case KNIGHT: positional_value = knight_pos_table[table_idx]; break;
            case BISHOP: positional_value = bishop_pos_table[table_idx]; break;
            case ROOK:   positional_value = rook_pos_table[table_idx]; break;
            case QUEEN:  positional_value = queen_pos_table[table_idx]; break;
            case KING:   positional_value = mg_king_table[table_idx]; break;
        }

        if (type == PAWN) {
            int mg_val = mg_pawn_pos_table[table_idx] * (1.0f - my_endgame_weight);
            int eg_val = eg_pawn_pos_table[table_idx] * my_endgame_weight;
            positional_value = mg_val + eg_val;
            std::cout << "Pawn positional value = mg_val + eg_val = " << mg_val << " + " << eg_val << " = " << positional_value << std::endl;
        } else if (type == KING) {
            int mg_val = mg_king_table[table_idx] * (1.0f - my_endgame_weight);
            int eg_val = eg_king_table[table_idx] * my_endgame_weight;
            positional_value = mg_val + eg_val;
            std::cout << "King positional value = mg_val + eg_val = " << mg_val << " + " << eg_val << " = " << positional_value << std::endl;
        }

        // Add both base material value and positional value
        side_score[color_idx] += positional_value;

    }
    
    int material_difference = side_score[player_idx] - side_score[opponent_idx];
    return material_difference; // + mop_up_score;
}

// The force_king_to_corner_endgame_eval function is currently not used in evaluate(),
// but its definition remains for future re-introduction if needed.
int mop_up_eval(Board *b, int my_material, int opponent_material, float endgame_weight) {
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

void get_materials(Board *b, int *my_material, int *opponent_material) {
    *my_material = 0;
    *opponent_material = 0;
    uint8_t my_colour = b->side_to_move ? BLACK : WHITE;
    uint8_t opp_colour = b->side_to_move ? WHITE : BLACK;
    for (Piece type = PAWN; type <= KING; type++) {
        U64 my_bb = b->bitboards[type | my_colour];
        U64 opp_bb = b->bitboards[type | opp_colour];
        *my_material += popcount(my_bb) * simple_base_values[type];
        *opponent_material += popcount(opp_bb) * simple_base_values[type];
    }
}

float endgame_phase_weight(int materials_no_pawns) {
    const float multiplier = 1.0f / endgame_mat_start;
    return 1.0f - std::min(1.0f, materials_no_pawns * multiplier);
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
