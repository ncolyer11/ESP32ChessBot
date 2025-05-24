#include <cmath>
#include <iomanip>

#include "eval.h"
#include "board.h"
#include "piece.h" 

int manhattan_distance(int square1, int square2);
int num_rook_moves_to_reach_square(int from, int to);
int mop_up_eval(Board *b, int my_material, int op_material, float endgame_weight, bool is_black);
void get_materials(Board *b, int *my_material, int *op_material);
float endgame_phase_weight(int materials_no_pawns);
void get_positional_values(Board *b, int *white_eval, int *black_eval, 
                           float white_endgame_weight, float black_endgame_weight);

int evaluate(Board *b) {
    int white_eval = 0;
    int black_eval = 0;

    int white_material = 0;
    int black_material = 0;
    get_materials(b, &white_material, &black_material);

    U64 white_pawn_bb = b->bitboards[piece_to_bb_idx(PAWN | WHITE)];
    U64 black_pawn_bb = b->bitboards[piece_to_bb_idx(PAWN | BLACK)];
    int white_mat_no_pawns = white_material - popcount(white_pawn_bb) * simple_base_values[PAWN];
    int black_mat_no_pawns = black_material - popcount(black_pawn_bb) * simple_base_values[PAWN];
    float white_endgame_weight = endgame_phase_weight(white_mat_no_pawns);
    float black_endgame_weight = endgame_phase_weight(black_mat_no_pawns);

    white_eval += white_material;
    black_eval += black_material;
    white_eval += mop_up_eval(b, white_material, black_material, black_endgame_weight, true);
    black_eval += mop_up_eval(b, black_material, white_material, white_endgame_weight, false);
    
    get_positional_values(b, &white_eval, &black_eval, white_endgame_weight, black_endgame_weight);

    int eval = white_eval - black_eval;
    return eval * (b->side_to_move ? -1 : 1); // Negate if black (1) to move

}

void get_positional_values(Board *b, int *white_eval, int *black_eval, 
                           float white_endgame_weight, float black_endgame_weight) {
    for (int sq_idx = 0; sq_idx < 64; sq_idx++) {
        Piece piece = b->squares[sq_idx];
        uint8_t type = piece & TYPE_MASK;
        bool is_black = (piece & BLACK) != 0;
        if (type == NONE) continue;
        
        // flip square index for white's perspective for PSTs as white is on the bottom
        int table_idx = is_black ? sq_idx : flip_square(sq_idx); 
        int positional_value = 0;
        if (type <= PAWN) { // king or pawn
            float eg_weight = is_black ? black_endgame_weight : white_endgame_weight;
            const int *mg_table = (type == PAWN) ? mg_pawn_pos_table : mg_king_table;
            const int *eg_table = (type == PAWN) ? eg_pawn_pos_table : eg_king_table;
            int mg_val = mg_table[table_idx] * (1.0f - eg_weight);
            int eg_val = eg_table[table_idx] * eg_weight;
            positional_value = mg_val + eg_val;
        } else {
            switch (type) {
                case KNIGHT: positional_value = knight_pos_table[table_idx]; break;
                case BISHOP: positional_value = bishop_pos_table[table_idx]; break;
                case ROOK:   positional_value = rook_pos_table[table_idx]; break;
                case QUEEN:  positional_value = queen_pos_table[table_idx]; break;
            }
        }
        
        if (is_black) {
            *black_eval += positional_value;
        } else {
            *white_eval += positional_value;
        }
    }
}

int mop_up_eval(Board *b, int my_material, int op_material, float endgame_weight, bool is_black) {
    int mop_up_score = 0;
    int friendly_colour = is_black ? BLACK : WHITE;
    int op_colour = is_black ? WHITE : BLACK;
    if (my_material >= op_material + pawn_threshold && endgame_weight > 0.0f) {
        U64 friendly_king_bb = b->bitboards[piece_to_bb_idx(KING | friendly_colour)];
        U64 op_king_bb = b->bitboards[piece_to_bb_idx(KING | op_colour)];
        int friendly_king_square = bit_scan_forward(friendly_king_bb);
        int op_king_square = bit_scan_forward(op_king_bb);

        mop_up_score += centre_manhattan_distance[op_king_square] * 10;
        mop_up_score += (7 - king_distance[friendly_king_square][op_king_square]) * 8;

        return static_cast<int>(mop_up_score * endgame_weight);
    }

    return 0;
}

void get_materials(Board *b, int *my_material, int *op_material) {
    *my_material = 0;
    *op_material = 0;
    uint8_t my_colour = b->side_to_move ? BLACK : WHITE;
    uint8_t opp_colour = b->side_to_move ? WHITE : BLACK;
    for (Piece type = KING; type <= QUEEN; type++) {
        U64 my_bb = b->bitboards[piece_to_bb_idx(type | my_colour)];
        U64 opp_bb = b->bitboards[piece_to_bb_idx(type | opp_colour)];
        int my_count = popcount(my_bb);
        int opp_count = popcount(opp_bb);
        int my_val = my_count * simple_base_values[type];
        int opp_val = opp_count * simple_base_values[type];

        *my_material += my_val;
        *op_material += opp_val;
    }
}

float endgame_phase_weight(int materials_no_pawns) {
    const float multiplier = 1.0f / endgame_mat_start;
    return 1.0f - std::min(1.0f, materials_no_pawns * multiplier);
}
