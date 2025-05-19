#include <cstdint>
#include <cstring>
#include <iostream>

#include "board.h"
#include "boardRepresentation.h"
#include "move.h"
#include "movegen.h"
#include "piece.h"
#include "util.h"

// Output arrays
Move move_list[MAX_MOVES];
int move_count = 0;
Move legal_move_list[MAX_MOVES];
int legal_move_count = 0;

// Precomputed attack bitboards
uint64_t pawn_attacks[2][64];
uint64_t knight_attacks[64];
uint64_t king_attacks[64];

uint64_t bitboard_for_piece(const Board* b, Piece piece) ;
std::string move_to_move_str(Move move);


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
std::string get_next_bot_move() {
    // Generate all legal moves
    generate_moves();
    filter_legal_moves();

    if (legal_move_count == 0) {
        std::cout << "Chesp32 (" << (board.side_to_move == 0 ? "White" : "Black") << ") has no legal moves." << std::endl;
        return "";
    }

    // std::cout << "Available legal moves: " << legal_move_count << ::endl;
    // for (int i = 0; i < legal_move_count; ++i) {
    //     std::string move_str = move_to_move_str(legal_move_list[i]);
    //     std::cout << "Legal move: " << move_str << std::endl;
    // }

    int random = std::rand();
    // std::cout << "Random num: " << random << std::endl;
    int idx = random % legal_move_count;
    Move chosen = legal_move_list[idx];

    int from = move_from(chosen), to = move_to(chosen), flag = move_flag(chosen);

    std::string move = square_idx_to_alg(from) + square_idx_to_alg(to);
    // Add promotion char if needed
    if (flag >= 3 && flag <= 6) {
        char promo_char;
        switch (flag) {
            case 3: promo_char = 'q'; break;
            case 4: promo_char = 'n'; break;
            case 5: promo_char = 'r'; break;
            case 6: promo_char = 'b'; break;
            default: promo_char = '?'; break;
        }
        move += promo_char;
    }

    std::string colour_to_move = (board.side_to_move == 0 ? "White" : "Black");
    std::cout << "Chesp32 (" << colour_to_move << ") plays: " << move << std::endl;
    return move;
}

/**
 * @brief Generates a list of all legal moves for the current board position, using the global 'board'.
 * 
 * Globals Used:
 *   - board (input): The current board state for which moves are generated.
 *   - move_list, move_count (output): Filled with all pseudo-legal moves found.
 *   - legal_move_list, legal_move_count (output): Populated with all legal moves after filtering.
 *
 * Side Effects:
 *   - Updates move_list, move_count, legal_move_list, and legal_move_count with current move information.
 * 
 * Usage:
 *   Call generate_moves() before move selection or perft, and after making any board changes.
 */
void generate_moves() {
    move_count = 0;
    generate_king_moves();
    generate_castling_moves();
    generate_sliding_moves(); // Queen, rook, bishop
    generate_knight_moves();
    generate_pawn_moves();
}

// Attack table initialization
void init_attack_bitboards() {
    // Knights
    for (int sq = 0; sq < 64; sq++) {
        knight_attacks[sq] = 0;
        int r = sq / 8, f = sq % 8;
        const int dr[8] = {1, 2, 2, 1, -1, -2, -2, -1};
        const int df[8] = {2, 1, -1, -2, -2, -1, 1, 2};
        for (int d = 0; d < 8; d++) {
            int nr = r + dr[d], nf = f + df[d];
            if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8)
                knight_attacks[sq] |= 1ULL << (nr*8+nf);
        }
    }
    // Kings
    for (int sq = 0; sq < 64; sq++) {
        king_attacks[sq] = 0;
        int r = sq / 8, f = sq % 8;
        for (int dr = -1; dr <= 1; dr++) {
            for (int df = -1; df <= 1; df++) {
                if (dr == 0 && df == 0) continue;
                int nr = r + dr, nf = f + df;
                if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8)
                    king_attacks[sq] |= 1ULL << (nr*8+nf);
            }
        }
    }
    // Pawns
    for (int sq = 0; sq < 64; sq++) {
        pawn_attacks[0][sq] = 0; // White
        pawn_attacks[1][sq] = 0; // Black
        int r = sq / 8, f = sq % 8;
        // White
        if (r < 7) {
            if (f > 0) pawn_attacks[0][sq] |= 1ULL << ((r+1)*8 + (f-1));
            if (f < 7) pawn_attacks[0][sq] |= 1ULL << ((r+1)*8 + (f+1));
        }
        // Black
        if (r > 0) {
            if (f > 0) pawn_attacks[1][sq] |= 1ULL << ((r-1)*8 + (f-1));
            if (f < 7) pawn_attacks[1][sq] |= 1ULL << ((r-1)*8 + (f+1));
        }
    }
}

// -- King moves (not castling) --
void generate_king_moves() {
    int king_sq = -1;
    Piece king_piece = KING | friendly_colour();
    for (int i = 0; i < 64; i++) {
        if (board.squares[i] == king_piece) {
            king_sq = i;
            break;
        }
    }
    if (king_sq == -1) return;
    uint64_t king_targets = king_attacks[king_sq];
    for (int t = 0; t < 64; t++)
        if ((king_targets & (1ULL << t)) && !is_friendly(board.squares[t]))
            add_move(king_sq, t, 0);
}

// -- Castling moves (call after king moves) --
void generate_castling_moves() {
    int king_sq = (friendly_colour() == WHITE) ? 4 : 60;
    // Kingside
    if ((friendly_colour() == WHITE && (board.castling_rights & 0b0001)) ||
        (friendly_colour() == BLACK && (board.castling_rights & 0b0100))) {
        int g = (friendly_colour() == WHITE) ? 5 : 61;
        int h = (friendly_colour() == WHITE) ? 6 : 62;
        if (board.squares[g] == NONE && board.squares[h] == NONE &&
            !king_in_check(&board, board.side_to_move) &&
            !king_in_check_after_move(king_sq, g) && !king_in_check_after_move(king_sq, h)) {
            add_move(king_sq, h, 2); // Castling flag
        }
    }
    // Queenside
    if ((friendly_colour() == WHITE && (board.castling_rights & 0b0010)) ||
        (friendly_colour() == BLACK && (board.castling_rights & 0b1000))) {
        int d = (friendly_colour() == WHITE) ? 3 : 59;
        int c = (friendly_colour() == WHITE) ? 2 : 58;
        int b = (friendly_colour() == WHITE) ? 1 : 57;
        if (board.squares[d] == NONE && board.squares[c] == NONE && board.squares[b] == NONE &&
            !king_in_check(&board, board.side_to_move) &&
            !king_in_check_after_move(king_sq, d) && !king_in_check_after_move(king_sq, c)) {
            add_move(king_sq, c, 2); // Castling flag
        }
    }
}

// -- Sliding piece moves (rooks, bishops, queens) --
void generate_sliding_moves() {
    static const int directions[8] = {1, -1, 8, -8, 9, -9, 7, -7};
    for (int i = 0; i < 64; i++) {
        Piece p = board.squares[i];
        if (!is_friendly(p)) continue;
        int type = p & TYPE_MASK;
        int start_dir = (type == ROOK) ? 0 : (type == BISHOP) ? 4 : 0;
        int end_dir = (type == ROOK) ? 4 : (type == BISHOP) ? 8 : (type == QUEEN) ? 8 : 0;
        if (type != ROOK && type != BISHOP && type != QUEEN) continue;
        for (int d = start_dir; d < end_dir; ++d) {
            int n = i;
            while (true) {
                int next = n + directions[d];
                if (next < 0 || next >= 64 || abs((n % 8) - (next % 8)) > 2) break;
                Piece target = board.squares[next];
                if (is_friendly(target)) break;
                add_move(i, next, 0);
                if (target != NONE) break;
                n = next;
            }
        }
    }
}

// -- Knight moves --
void generate_knight_moves() {
    for (int i = 0; i < 64; i++) {
        if (board.squares[i] != (KNIGHT | friendly_colour())) continue;
        uint64_t attacks = knight_attacks[i];
        for (int t = 0; t < 64; t++)
            if ((attacks & (1ULL << t)) && !is_friendly(board.squares[t]))
                add_move(i, t, 0);
    }
}

// -- Pawn moves (single, double, capture, en passant, promotion) --
void generate_pawn_moves() {
    int direction = board.side_to_move ? -8 : 8;
    int start_rank = board.side_to_move ? 6 : 1;
    int promotion_rank = board.side_to_move ? 0 : 7;
    for (int i = 0; i < 64; i++) {
        if (board.squares[i] != (PAWN | friendly_colour())) continue;
        int to = i + direction;
        // Forward
        if (to >= 0 && to < 64 && board.squares[to] == NONE) {
            if (to / 8 == promotion_rank) {
                add_move(i, to, 3); add_move(i, to, 4);
                add_move(i, to, 5); add_move(i, to, 6);
            } else {
                add_move(i, to, 0);
                // Double push from start
                if (i / 8 == start_rank) {
                    int dbl = i + 2 * direction;
                    if (board.squares[dbl] == NONE)
                        add_move(i, dbl, 7);
                }
            }
        }
        // Captures
        for (int df = -1; df <= 1; df += 2) {
            int cap = i + direction + df;
            if (cap < 0 || cap >= 64) continue;
            if (board.squares[cap] != NONE && is_opponent(board.squares[cap])) {
                if (cap / 8 == promotion_rank) {
                    add_move(i, cap, 3); add_move(i, cap, 4);
                    add_move(i, cap, 5); add_move(i, cap, 6);
                } else {
                    add_move(i, cap, 0);
                }
            }
        }
        // En passant
        if (board.enpassant_square < 64) {
            for (int df = -1; df <= 1; df += 2) {
                if (i % 8 + df < 0 || i % 8 + df > 7) continue;
                int cap = i + direction + df;
                if (cap == board.enpassant_square) {
                    add_move(i, cap, 1);
                }
            }
        }
    }
}

// Additional helper (you must provide) to check if a king is attacked after moving to a square
bool king_in_check_after_move(int from, int to) {
    Board temp = board;
    temp.squares[to] = temp.squares[from];
    temp.squares[from] = NONE;
    return king_in_check(&temp, board.side_to_move);
}

// -- Legal move filtering --
// Writes all legal moves from the global move_list/move_count into legal_move_list/legal_move_count
void filter_legal_moves() {
    legal_move_count = 0;
    for (int i = 0; i < move_count; i++) {
        Board temp = board; // copy the global board
        std::string move_str = move_to_move_str(move_list[i]);
        // std::cout << "[filter_legal_moves] Testing move: " << move_str << std::endl;
        make_move_on_board(&temp, move_list[i]);
        if (!king_in_check(&temp, board.side_to_move)) {
            // std::cout << "[filter_legal_moves] Move is legal: " << move_str << std::endl;
            legal_move_list[legal_move_count++] = move_list[i];
        } else {
            // std::cout << "[filter_legal_moves] Move is illegal (king in check): " << move_str << std::endl;
        }
    }
    // std::cout << "[filter_legal_moves] Total legal moves: " << legal_move_count << std::endl;
}

void make_move_on_board(Board* b, Move move) {
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
        // Double pawn move
        if (flag == 7) {
            b->enpassant_square = (from + to) / 2;
        } else {
            b->enpassant_square = 64;
        }
    }
    update_castling_rights(b, from, to, piece, captured);

    // Clocks
    if ((piece & TYPE_MASK) == PAWN || captured != NONE)
        b->half_move_clock = 0;
    else
        ++b->half_move_clock;
    if (!(b->side_to_move)) b->full_moves++;
    b->side_to_move = !b->side_to_move;
}

// -- Helpers --

void apply_normal_move(Board* b, int from, int to, Piece piece, Piece captured) {
    b->squares[to] = piece;
    b->squares[from] = NONE;
    b->bitboards[piece_to_bitboard_idx(piece)] |= (1ULL << to);
    b->bitboards[piece_to_bitboard_idx(piece)] &= ~(1ULL << from);
    if (captured != NONE) {
        b->bitboards[piece_to_bitboard_idx(captured)] &= ~(1ULL << to);
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
    b->bitboards[piece_to_bitboard_idx(piece)] &= ~(1ULL << from);
    b->bitboards[piece_to_bitboard_idx(promo)] |= (1ULL << to);
}

void apply_en_passant_move(Board* b, int from, int to, Piece piece) {
    b->squares[to] = piece;
    b->squares[from] = NONE;
    b->bitboards[piece_to_bitboard_idx(piece)] |= (1ULL << to);
    b->bitboards[piece_to_bitboard_idx(piece)] &= ~(1ULL << from);
    int ep_capture_sq = to + (b->side_to_move ? 8 : -8);
    b->bitboards[piece_to_bitboard_idx(PAWN | opponent_colour())] &= ~(1ULL << ep_capture_sq);
    b->squares[ep_capture_sq] = NONE;
}

void apply_castling_move(Board* b, int from, int to, Piece piece) {
    b->squares[to] = piece;
    b->squares[from] = NONE;
    b->bitboards[piece_to_bitboard_idx(piece)] |= (1ULL << to);
    b->bitboards[piece_to_bitboard_idx(piece)] &= ~(1ULL << from);
    // Move rook
    if (to == 6) { // White kingside
        b->squares[5] = b->squares[7];
        b->squares[7] = NONE;
        b->bitboards[piece_to_bitboard_idx(ROOK | WHITE)] |= (1ULL << 5);
        b->bitboards[piece_to_bitboard_idx(ROOK | WHITE)] &= ~(1ULL << 7);
    } else if (to == 2) { // White queenside
        b->squares[3] = b->squares[0];
        b->squares[0] = NONE;
        b->bitboards[piece_to_bitboard_idx(ROOK | WHITE)] |= (1ULL << 3);
        b->bitboards[piece_to_bitboard_idx(ROOK | WHITE)] &= ~(1ULL << 0);
    } else if (to == 62) { // Black kingside
        b->squares[61] = b->squares[63];
        b->squares[63] = NONE;
        b->bitboards[piece_to_bitboard_idx(ROOK | BLACK)] |= (1ULL << 61);
        b->bitboards[piece_to_bitboard_idx(ROOK | BLACK)] &= ~(1ULL << 63);
    } else if (to == 58) { // Black queenside
        b->squares[59] = b->squares[56];
        b->squares[56] = NONE;
        b->bitboards[piece_to_bitboard_idx(ROOK | BLACK)] |= (1ULL << 59);
        b->bitboards[piece_to_bitboard_idx(ROOK | BLACK)] &= ~(1ULL << 56);
    }
}

void update_castling_rights(Board* b, int from, int to, Piece piece, Piece captured) {
    // If king moved
    if ((piece & TYPE_MASK) == KING) {
        if (piece & WHITE) b->castling_rights &= 0b1100;
        else b->castling_rights &= 0b0011;
    }
    // If rook moved
    if ((piece & TYPE_MASK) == ROOK) {
        if (from == 0) b->castling_rights &= ~0b0010;
        else if (from == 7) b->castling_rights &= ~0b0001;
        else if (from == 56) b->castling_rights &= ~0b1000;
        else if (from == 63) b->castling_rights &= ~0b0100;
    }
    // If rook captured
    if ((captured & TYPE_MASK) == ROOK) {
        if (to == 0) b->castling_rights &= ~0b0010;
        else if (to == 7) b->castling_rights &= ~0b0001;
        else if (to == 56) b->castling_rights &= ~0b1000;
        else if (to == 63) b->castling_rights &= ~0b0100;
    }
}

// -- King in check detection (uses attack bitboards) --
bool king_in_check(const Board* b, bool side_to_move) {
    Piece king = KING | (side_to_move ? BLACK : WHITE);
    int king_sq = -1;
    for (int i = 0; i < 64; i++)
        if (b->squares[i] == king) { king_sq = i; break; }
    if (king_sq == -1) {
        // std::cout << "[king_in_check] King not found for side " << (side_to_move ? "Black" : "White") << std::endl;
        return false;
    }
    int opp = side_to_move ? 0 : 1;

    // Pawn attacks
    if (pawn_attacks[opp][king_sq] & bitboard_for_piece(b, PAWN | opponent_colour())) {
        // std::cout << "[king_in_check] King at " << square_idx_to_alg(king_sq) << " is attacked by pawn" << std::endl;
        return true;
    }
    // Knight attacks
    if (knight_attacks[king_sq] & bitboard_for_piece(b, KNIGHT | opponent_colour())) {
        // std::cout << "[king_in_check] King at " << square_idx_to_alg(king_sq) << " is attacked by knight" << std::endl;
        return true;
    }
    // King attacks (for double check detection)
    if (king_attacks[king_sq] & bitboard_for_piece(b, KING | opponent_colour())) {
        // std::cout << "[king_in_check] King at " << square_idx_to_alg(king_sq) << " is attacked by king" << std::endl;
        return true;
    }
    // Sliding piece attacks (rook, bishop, queen)
    // Brute-force direction scan
    static const int dirs[8] = {1, -1, 8, -8, 9, -9, 7, -7};
    for (int d = 0; d < 8; d++) {
        int n = king_sq;
        while (true) {
            n += dirs[d];
            if (n < 0 || n >= 64 || abs((n % 8) - (king_sq % 8)) > 2) break;
            Piece target = b->squares[n];
            if (target == NONE) continue;
            uint8_t opp_color = side_to_move ? WHITE : BLACK;
            if ((target & COLOUR_MASK) == opp_color) {
                int typ = target & TYPE_MASK;
                if (((d < 4) && (typ == ROOK || typ == QUEEN)) || // Orthogonal
                    ((d >= 4) && (typ == BISHOP || typ == QUEEN))) { // Diagonal
                    // std::cout << "[king_in_check] King at " << square_idx_to_alg(king_sq)
                    //           << " is attacked by " << ((typ == ROOK) ? "rook" : (typ == BISHOP) ? "bishop" : "queen")
                    //           << " at " << square_idx_to_alg(n) << std::endl;
                    return true;
                }
            }
            break;
        }
    }
    // std::cout << "[king_in_check] King at " << square_idx_to_alg(king_sq) << " is not in check" << std::endl;
    return false;
}

// Helper: return bitboard for all pieces of a type and color
uint64_t bitboard_for_piece(const Board* b, Piece piece) {
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
