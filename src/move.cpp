#include <iostream>

#include "board.h"
#include "boardRepresentation.h"
#include "move.h"
#include "piece.h"
#include "util.h"

U64 bitboard_for_piece(const Board* b, Piece piece);

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
bool make_move_on_board(Board* b, Move move) {
    if (!validate_move_encoding(move)) {
        std::cout << "[make_move_on_board] Invalid move encoding: " << move << std::endl;
        return false;
    }
    int from = move_from(move), to = move_to(move), flag = move_flag(move);
    Piece piece = b->squares[from];
    Piece captured = b->squares[to];
    if (flag >= 3 && flag <= 6) {
        apply_promotion_move(b, from, to, piece, flag);
    } else if (flag == 1) {
        apply_en_passant_move(b, from, to, piece);
    } else if (flag == 2) {
        apply_castling_move(b, from, to, piece);
    } else {
        apply_normal_move(b, from, to, piece, captured);
        // Double pawn move, i.e. pawn has moved 2 ranks
        if (abs(to / 8 - from / 8) == 2) {
            b->enpassant_square = (from + to) / 2;
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

// -- Helpers --

void apply_normal_move(Board* b, int from, int to, Piece piece, Piece captured) {
    b->squares[to] = piece;
    b->squares[from] = NONE;
    b->bitboards[piece_to_bitboard_idx(piece)] ^= (1ULL << from) | (1ULL << to);
    if (captured != NONE) {
        b->bitboards[piece_to_bitboard_idx(captured)] ^= (1ULL << to);
    }
}

void apply_promotion_move(Board* b, int from, int to, Piece piece, int flag) {
    Piece promo = 0;
    switch (flag) {
        case 3: promo = QUEEN; break;
        case 4: promo = KNIGHT; break;
        case 5: promo = ROOK; break;
        case 6: promo = BISHOP; break;
    }
    promo |= (piece & COLOUR_MASK);
    b->squares[to] = promo;
    b->squares[from] = NONE;
    b->bitboards[piece_to_bitboard_idx(piece)] ^= (1ULL << from);
    b->bitboards[piece_to_bitboard_idx(promo)] |= (1ULL << to);
}

void apply_en_passant_move(Board* b, int from, int to, Piece piece) {
    b->squares[to] = piece;
    b->squares[from] = NONE;
    b->bitboards[piece_to_bitboard_idx(piece)] ^= (1ULL << from) | (1ULL << to);
    int ep_capture_sq = to + (b->side_to_move ? 8 : -8);
    b->bitboards[piece_to_bitboard_idx(PAWN | opponent_colour())] ^= (1ULL << ep_capture_sq);
    b->squares[ep_capture_sq] = NONE;
}

void apply_castling_move(Board* b, int from, int to, Piece piece) {
    b->squares[to] = piece;
    b->squares[from] = NONE;
    b->bitboards[piece_to_bitboard_idx(piece)] ^= (1ULL << from) | (1ULL << to);
    // Move rook
    if (to == 6) { // White kingside
        b->squares[5] = b->squares[7];
        b->squares[7] = NONE;
        b->bitboards[piece_to_bitboard_idx(ROOK | WHITE)] ^= (1ULL << 5) | (1ULL << 7);
    } else if (to == 2) { // White queenside
        b->squares[3] = b->squares[0];
        b->squares[0] = NONE;
        b->bitboards[piece_to_bitboard_idx(ROOK | WHITE)] ^= (1ULL << 3) | (1ULL << 0);
    } else if (to == 62) { // Black kingside
        b->squares[61] = b->squares[63];
        b->squares[63] = NONE;
        b->bitboards[piece_to_bitboard_idx(ROOK | BLACK)] ^= (1ULL << 61) | (1ULL << 63);
    } else if (to == 58) { // Black queenside
        b->squares[59] = b->squares[56];
        b->squares[56] = NONE;
        b->bitboards[piece_to_bitboard_idx(ROOK | BLACK)] ^= (1ULL << 59) | (1ULL << 56);
    }
}

void update_castling_rights(Board* b, int from, int to, Piece piece, Piece captured) {
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
bool king_in_check(const Board* b, bool side_to_move) {
    Piece king = KING | (side_to_move ? BLACK : WHITE);
    U64 king_bb = b->bitboards[piece_to_bitboard_idx(king)];
    if (king_bb == 0) {
        // std::cout << "[king_in_check] King not found for side "
                //   << (side_to_move ? "Black" : "White") << std::endl;
        return false;
    }
    int king_sq = bit_scan_forward(king_bb);
    // Pawn attacks
    if (pawn_attacks[side_to_move][king_sq] & bitboard_for_piece(b, PAWN | opponent_colour())) {
        // std::cout << "[king_in_check] King at " << square_idx_to_alg(king_sq) 
                //   << " is attacked by pawn" << std::endl;
        return true;
    }
    // Knight attacks
    if (knight_attacks[king_sq] & bitboard_for_piece(b, KNIGHT | opponent_colour())) {
        // std::cout << "[king_in_check] King at " << square_idx_to_alg(king_sq) 
                //   << " is attacked by knight" << std::endl;
        return true;
    }
    // King attacks (for double check detection)
    if (king_attacks[king_sq] & bitboard_for_piece(b, KING | opponent_colour())) {
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
            new_sq += dirs[dir];
            if (new_sq < 0 || new_sq >= 64 || abs((new_sq % 8) - (king_sq % 8)) > 2) break;
            Piece target = b->squares[new_sq];
            if (target == NONE) continue;
            // Has found piece so will need to break after is it'll be blocking or attacking
            uint8_t opp_color = side_to_move ? WHITE : BLACK;
            if ((target & COLOUR_MASK) == opp_color) {
                int type = target & TYPE_MASK;
                const char *attacking_piece;
                if (type == QUEEN) attacking_piece = "queen";
                else if (type == ROOK && dir < 4) attacking_piece = "rook";
                else if (type == BISHOP && dir >= 4) attacking_piece = "bishop";
                else break;
                (void)attacking_piece;
                // std::cout << "[king_in_check] King at " << square_idx_to_alg(king_sq)
                //             << " is attacked by " << attacking_piece
                //             << " at " << square_idx_to_alg(new_sq) << std::endl;
                return true;
            }
            break;
        }
    }
    // std::cout << "[king_in_check] King at " << square_idx_to_alg(king_sq) << " is not in check" << std::endl;
    return false;
}

// Return the bitboard for specific coloured piece
U64 bitboard_for_piece(const Board* b, Piece piece) {
    return b->bitboards[piece_to_bitboard_idx(piece)];
}

std::string move_to_move_str(Move move) {
    int from = move_from(move);
    int to = move_to(move);
    int flag = move_flag(move);
    std::string move_str = square_idx_to_alg(from) + square_idx_to_alg(to);
    if (flag >= 3 && flag <= 6) {
        char promo_char;
        switch (flag) {
            case 3: promo_char = 'q'; break;
            case 4: promo_char = 'n'; break;
            case 5: promo_char = 'r'; break;
            case 6: promo_char = 'b'; break;
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
            case 'q': flag = 3; break;
            case 'n': flag = 4; break;
            case 'r': flag = 5; break;
            case 'b': flag = 6; break;
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
    if (from < 0 || from >= 64 || to < 0 || to >= 64)
        return false;

    // Check that flag is within expected range (0-7 for standard moves, 3-6 for promotions)
    if (flag < 0 || flag > 6)
        return false;

    return true;
}
