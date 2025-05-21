#include <iostream>

#include "board.h"
#include "boardRepresentation.h"
#include "move.h"
#include "piece.h"
#include "util.h"

U64 bitboard_for_piece(const Board* b, Piece piece);

void apply_normal_move(Board *b, int from, int to, Piece piece, Piece captured);
void apply_promotion_move(Board *b, int from, int to, Piece piece, Piece captured, int flag);
void apply_en_passant_move(Board *b, int from, int to, Piece piece);
void apply_castling_move(Board *b, int from, int to, Piece piece);
void update_castling_rights(Board *b, int from, int to, Piece piece, Piece captured);

void undo_normal_move(Board *b, int from, int to, Piece moved, Piece captured);
void undo_promotion_move(Board *b, int from, int to, Piece moved, Piece captured);
void undo_en_passant_move(Board *b, int from, int to, Piece moved);
void undo_castling_move(Board *b, int from, int to, Piece moved);

int partition(Move *moves, int *scores, int low, int high);


/** TODO: UPDATE TO DOCSTRING TO MATCH NEW MAKE MOVE FUNC
 * @brief Modifies the internal board state based on the move provided.
 *
 * This function updates the board state, including the side to move, castling rights,
 * en passant square, half-move clock, and full move number.
 * 
 * The function also checks for checkmate and stalemate conditions after the move and ends the game
 * accordingly.
 * @param b The board struct, passed by reference, to make the move on
 * @param move The move to be made in algebraic notation (e.g., "d2d4", or e7e8q for a promotion).
 * 
 * @return true if the move was made successfully
 */
bool make_move_on_board(Board *b, Move move) {
    if (!validate_move_encoding(move)) {
        std::cout << "[make_move_on_board] Invalid move encoding: " << move << std::endl;
        return false;
    }
    int from = move_from(move), to = move_to(move), flag = move_flag(move);
    Piece piece = b->squares[from];
    Piece captured = b->squares[to];
    b->undo_state = {
        captured,
        b->castling_rights,
        b->enpassant_square,
        b->half_move_clock,
        b->full_moves
    };

    if (flag >= PROMOTION_QUEEN_FLAG && flag <= PROMOTION_BISHOP_FLAG) {
        apply_promotion_move(b, from, to, piece, captured, flag);
    } else if (flag == ENPASSANT_FLAG) {
        apply_en_passant_move(b, from, to, piece);
    } else if (flag == CASTLING_FLAG) {
        apply_castling_move(b, from, to, piece);
    } else {
        apply_normal_move(b, from, to, piece, captured);
        // Double pawn move, i.e. pawn has moved 2 ranks
        if (flag == DOUBLE_PAWN_FLAG) {
            b->enpassant_square = (from + to) / 2; // one square behind it
        } else {
            b->enpassant_square = 64;
        }
    }
    update_castling_rights(b, from, to, piece, captured);

    // Clocks
    if ((piece & TYPE_MASK) == PAWN || captured != NONE) {
        b->half_move_clock = 0;
    } else {
        ++b->half_move_clock;
    }
    if (!(b->side_to_move)) b->full_moves++;
    b->side_to_move ^= 1;

    return true;
}

/**
 * @brief Reverts the last move made on the board.
 * 
 * This function restores the board state to its previous configuration
 * by undoing the last move. It also restores the castling rights, en passant square,
 * half-move clock, and full move number to their previous values.
 * 
 * @note This function assumes that the move to be undone is valid and was made
 *       on the board. It does not perform any validation checks.
 * 
 * @param b The board struct, passed by reference, to unmake the move on
 * @param move The move to be undone in algebraic notation (e.g., "d2d4", or e7e8q for a promotion).
 */
void unmake_move_on_board(Board *b, Move move) {
    int from = move_from(move);
    int to = move_to(move);
    int flag = move_flag(move);

    Piece moved = b->squares[to];
    Piece captured = b->undo_state.captured_piece;

    // Reverse piece‐placement
    if (flag >= PROMOTION_QUEEN_FLAG && flag <= PROMOTION_BISHOP_FLAG) {
        undo_promotion_move(b, from, to, moved, captured);
    } else if (flag == ENPASSANT_FLAG) {
        undo_en_passant_move(b, from, to, moved);
    } else if (flag == CASTLING_FLAG) {
        undo_castling_move(b, from, to, moved);
    } else {
        undo_normal_move(b, from, to, moved, captured);
    }

    // Restore params
    b->castling_rights = b->undo_state.castling_rights;
    b->enpassant_square = b->undo_state.enpassant_square;
    b->half_move_clock = b->undo_state.half_move_clock;
    b->full_moves = b->undo_state.full_moves;
    b->side_to_move ^= 1;
}

// -- Helpers --
void apply_normal_move(Board *b, int from, int to, Piece piece, Piece captured) {
    b->squares[to] = piece;
    b->squares[from] = NONE;
    b->bitboards[piece_to_bitboard_idx(piece)] ^= (1ULL << from) | (1ULL << to);
    if (captured != NONE) {
        b->bitboards[piece_to_bitboard_idx(captured)] ^= (1ULL << to);
    }
}

void apply_promotion_move(Board *b, int from, int to, Piece piece, Piece captured, int flag) {
    Piece promo = 0;
    switch (flag) {
        case PROMOTION_QUEEN_FLAG: promo = QUEEN; break;
        case PROMOTION_KNIGHT_FLAG: promo = KNIGHT; break;
        case PROMOTION_ROOK_FLAG: promo = ROOK; break;
        case PROMOTION_BISHOP_FLAG: promo = BISHOP; break;
    }
    promo |= (piece & COLOUR_MASK);
    b->squares[to] = promo;
    b->squares[from] = NONE;
    b->bitboards[piece_to_bitboard_idx(piece)] ^= (1ULL << from);
    b->bitboards[piece_to_bitboard_idx(promo)] |= (1ULL << to);
    // Check if pawn captured whilst promoting
    if (captured != NONE) {
        b->bitboards[piece_to_bitboard_idx(captured)] ^= (1ULL << to);
    }
}

void apply_en_passant_move(Board *b, int from, int to, Piece piece) {
    b->squares[to] = piece;
    b->squares[from] = NONE;
    b->bitboards[piece_to_bitboard_idx(piece)] ^= (1ULL << from) | (1ULL << to);
    int ep_capture_sq = to + (b->side_to_move ? 8 : -8);
    b->bitboards[piece_to_bitboard_idx(PAWN | opponent_colour(b))] ^= (1ULL << ep_capture_sq);
    b->squares[ep_capture_sq] = NONE;
    b->enpassant_square = 64;
}

void apply_castling_move(Board *b, int from, int to, Piece king) {
    // std::cout << "[apply_castling_move] King moves from " << square_idx_to_alg(from)
    //           << " to " << square_idx_to_alg(to) << std::endl;
    b->squares[to] = king;
    b->squares[from] = NONE;
    b->bitboards[piece_to_bitboard_idx(king)] ^= (1ULL << from) | (1ULL << to);

    // Move rook
    if (to == 6) { // White kingside
        // std::cout << "[apply_castling_move] White kingside rook moves from h1 to f1" << std::endl;
        b->squares[5] = (ROOK | WHITE);
        b->squares[7] = NONE;
        b->bitboards[piece_to_bitboard_idx(ROOK | WHITE)] ^= (1ULL << 5) | (1ULL << 7);
    } else if (to == 2) { // White queenside
        // std::cout << "[apply_castling_move] White queenside rook moves from a1 to d1" << std::endl;
        b->squares[3] = (ROOK | WHITE);
        b->squares[0] = NONE;
        b->bitboards[piece_to_bitboard_idx(ROOK | WHITE)] ^= (1ULL << 3) | (1ULL << 0);
    } else if (to == 62) { // Black kingside
        // std::cout << "[apply_castling_move] Black kingside rook moves from h8 to f8" << std::endl;
        b->squares[61] = (ROOK | BLACK);
        b->squares[63] = NONE;
        b->bitboards[piece_to_bitboard_idx(ROOK | BLACK)] ^= (1ULL << 61) | (1ULL << 63);
    } else if (to == 58) { // Black queenside
        // std::cout << "[apply_castling_move] Black queenside rook moves from a8 to d8" << std::endl;
        b->squares[59] = (ROOK | BLACK);
        b->squares[56] = NONE;
        b->bitboards[piece_to_bitboard_idx(ROOK | BLACK)] ^= (1ULL << 59) | (1ULL << 56);
    }
}

void update_castling_rights(Board *b, int from, int to, Piece piece, Piece captured) {
    // If king moved
    if ((piece & TYPE_MASK) == KING) {
        switch (piece & COLOUR_MASK) {
            case WHITE: b->castling_rights &= 0b1100; break;
            case BLACK: b->castling_rights &= 0b0011;
        }
    }
    // If rook moved
    if ((piece & TYPE_MASK) == ROOK) {
        switch (from) {
            case 0:  b->castling_rights &= ~0b0010; break;
            case 7:  b->castling_rights &= ~0b0001; break;
            case 56: b->castling_rights &= ~0b1000; break;
            case 63: b->castling_rights &= ~0b0100;
        }
    }
    // If rook captured
    if ((captured & TYPE_MASK) == ROOK) {
        switch (to) {
            case 0:  b->castling_rights &= ~0b0010; break;
            case 7:  b->castling_rights &= ~0b0001; break;
            case 56: b->castling_rights &= ~0b1000; break;
            case 63: b->castling_rights &= ~0b0100;
        }
    }
}

// -- King in check detection (uses attack bitboards) --
bool king_in_check(Board *b) {
    bool side_to_move = b->side_to_move;
    Piece king = KING | (side_to_move ? BLACK : WHITE);
    U64 king_bb = b->bitboards[piece_to_bitboard_idx(king)];
    if (king_bb == 0) {
        // std::cout << "[king_in_check] King not found for side "
                //   << (side_to_move ? "Black" : "White") << std::endl;
        return false;
    }
    int king_sq = bit_scan_forward(king_bb);
    // Pawn attacks
    if (pawn_attacks[side_to_move][king_sq] & bitboard_for_piece(b, PAWN | opponent_colour(b))) {
        // std::cout << "[king_in_check] King at " << square_idx_to_alg(king_sq) 
                //   << " is attacked by pawn" << std::endl;
        return true;
    }
    // Knight attacks
    if (knight_attacks[king_sq] & bitboard_for_piece(b, KNIGHT | opponent_colour(b))) {
        // std::cout << "[king_in_check] King at " << square_idx_to_alg(king_sq) 
                //   << " is attacked by knight" << std::endl;
        return true;
    }
    // King attacks (for double check detection)
    if (king_attacks[king_sq] & bitboard_for_piece(b, KING | opponent_colour(b))) {
        // std::cout << "[king_in_check] King at " << square_idx_to_alg(king_sq)
                //   << " is attacked by king" << std::endl;
        return true;
    }
    // Sliding piece attacks (rook, bishop, queen)
    // Brute-force direction scan using offsets to get to adjacent square indices
    static const int dirs[8] = {1, -1, 8, -8, 9, -9, 7, -7};
    for (int dir = 0; dir < 8; dir++) {
        int new_sq = king_sq;
        while (true) {
            int file_before = new_sq % 8;
            new_sq += dirs[dir];
            if (new_sq < 0 || new_sq >= 64) break;
            int file_after = new_sq % 8;
            if (abs(file_after - file_before) > 1) break;

            Piece target = b->squares[new_sq];
            if (target == NONE) continue;
            uint8_t opp_color = side_to_move ? WHITE : BLACK;
            int type = target & TYPE_MASK;
            const char *attacking_piece = nullptr;
            if ((target & COLOUR_MASK) == opp_color) {
                if (type == QUEEN) attacking_piece = "queen";
                else if (type == ROOK && dir < 4) attacking_piece = "rook";
                else if (type == BISHOP && dir >= 4) attacking_piece = "bishop";
                if (attacking_piece) {
                    (void)attacking_piece;
                    // std::cout << "[king_in_check] King at " << square_idx_to_alg(king_sq)
                    //         << " is attacked by " << attacking_piece
                    //         << " at " << square_idx_to_alg(new_sq) << std::endl;
                    return true;
                }
            }
            break;
        }
    }
    // std::cout << "[king_in_check] King at " << square_idx_to_alg(king_sq) << " is not in check" << std::endl;
    return false;
}

// Return the bitboard for specific coloured piece
U64 bitboard_for_piece(const Board *b, Piece piece) {
    return b->bitboards[piece_to_bitboard_idx(piece)];
}

std::string move_to_move_str(Move move) {
    int from = move_from(move);
    int to = move_to(move);
    int flag = move_flag(move);
    std::string move_str = square_idx_to_alg(from) + square_idx_to_alg(to);
    if (flag >= PROMOTION_QUEEN_FLAG && flag <= PROMOTION_BISHOP_FLAG) {
        char promo_char;
        switch (flag) {
            case PROMOTION_QUEEN_FLAG: promo_char = 'q'; break;
            case PROMOTION_KNIGHT_FLAG: promo_char = 'n'; break;
            case PROMOTION_ROOK_FLAG: promo_char = 'r'; break;
            case PROMOTION_BISHOP_FLAG: promo_char = 'b'; break;
            default: promo_char = '?'; break;
        }
        move_str += promo_char;
    }
    return move_str;
}

Move move_str_to_move(std::string move_str) {
    if (move_str.length() != 4 && move_str.length() != 5) {
        std::cout << "[move_str_to_move] Invalid move string length: " << move_str << std::endl;
        return INVALID_MOVE;
    }
    int from = alg_to_square_idx(move_str.substr(0, 2));
    int to = alg_to_square_idx(move_str.substr(2, 2));
    int flag = 0;
    if (move_str.length() == 5) {
        switch (move_str[4]) {
            case 'q': flag = PROMOTION_QUEEN_FLAG; break;
            case 'n': flag = PROMOTION_KNIGHT_FLAG; break;
            case 'r': flag = PROMOTION_ROOK_FLAG; break;
            case 'b': flag = PROMOTION_BISHOP_FLAG; break;
            default: flag = 0; break;
        }
    }

    return encode_move(from, to, flag);
}

bool validate_move_encoding(Move move) {
    int from = move_from(move);
    int to = move_to(move);
    int flag = move_flag(move);

    // Check that from and to are valid square indices (0-63)
    if (from < 0 || from >= 64 || to < 0 || to >= 64) {
        return false;
    }
    // Check that flag is within expected range (0-7 for standard moves, 3-6 for promotions)
    if (flag < 0 || flag > 7) {
        return false;
    }

    return true;
}

// UNDO helpers, reverse of apply_*
void undo_normal_move(Board *b, int from, int to, Piece moved, Piece captured) {
    // 'Uncapture' the piece/none
    b->squares[from] = moved;
    b->squares[to] = captured;
    b->bitboards[piece_to_bitboard_idx(moved)] ^= (1ULL << from) | (1ULL << to);
    if (captured != NONE) {
        b->bitboards[piece_to_bitboard_idx(captured)] |= (1ULL << to);
    }
}

void undo_promotion_move(Board *b, int from, int to, Piece moved, Piece captured) {
    // moved is the promoted piece; original pawn = (PAWN|colour)
    Piece pawn = PAWN | (moved & COLOUR_MASK);
    b->squares[from] = pawn;
    b->squares[to] = captured;
    // remove promo from bitboard, restore pawn
    b->bitboards[piece_to_bitboard_idx(moved)] ^= (1ULL << to);
    b->bitboards[piece_to_bitboard_idx(pawn)] |= (1ULL << from);
    if (captured != NONE) {
        b->bitboards[piece_to_bitboard_idx(captured)] |= (1ULL << to);
    }
}

void undo_en_passant_move(Board *b, int from, int to, Piece moved) {
    bool was_black = (moved & BLACK);
    int ep_capture_sq = to + (was_black ? 8 : -8);
    // restore pawn capture
    b->squares[from] = moved;
    b->squares[to] = NONE;
    b->squares[ep_capture_sq] = PAWN | (was_black ? WHITE : BLACK);
    // restore bitboards
    b->bitboards[piece_to_bitboard_idx(moved)] ^= (1ULL << from) | (1ULL << to);
    int capture_bb_idx = piece_to_bitboard_idx(PAWN | (was_black ? WHITE : BLACK));
    b->bitboards[capture_bb_idx] |= (1ULL << ep_capture_sq);
}

void undo_castling_move(Board *b, int from, int to, Piece king) {
    // move king back
    b->squares[from] = king;
    b->squares[to] = NONE;
    b->bitboards[piece_to_bitboard_idx(king)] ^= (1ULL << from) | (1ULL << to);

    // return rook
    int rook_from, rook_to;
    if (to == 6) { // White kingside
        rook_from = 7;
        rook_to = 5;
    } else if (to == 2) { // White queenside
        rook_from = 0;
        rook_to = 3;
    } else if (to == 62) { // Black kingside
        rook_from = 63;
        rook_to = 61;
    } else if (to == 58) { // Black queenside
        rook_from = 56;
        rook_to = 59;
    } else {
        rook_from = rook_to = -1; // Invalid, should not happen
    }
    Piece rook = ROOK | (king & COLOUR_MASK);
    b->squares[rook_from] = rook;
    b->squares[rook_to] = NONE;
    b->bitboards[piece_to_bitboard_idx(rook)] ^= (1ULL << rook_from) | (1ULL << rook_to);
}

// quicksort function
// Partition function for parallel sorting of moves and scores
int partition(Move* moves, int* scores, int low, int high) {
    int pivot = scores[high]; // use the score at high as the pivot
    int i = low - 1;
    for (int j = low; j < high; j++) {
        // Change > to < for ascending order
        if (scores[j] > pivot) { // Sort descending
            i++;
            std::swap(moves[i], moves[j]);
            std::swap(scores[i], scores[j]);
        }
    }
    std::swap(moves[i + 1], moves[high]);
    std::swap(scores[i + 1], scores[high]);
    return i + 1;
}

void quicksort_moves(Move* moves, int* scores, int low, int high) {
    if (low < high) {
        int pi = partition(moves, scores, low, high);
        quicksort_moves(moves, scores, low, pi - 1);
        quicksort_moves(moves, scores, pi + 1, high);
    }
}
