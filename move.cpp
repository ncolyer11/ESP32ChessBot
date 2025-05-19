#include <iostream>

#include "board.h"
#include "move.h"
#include "piece.h"
#include "util.h"

bool check_end_game_conditions();

// Request next move from the current player
std::string get_next_player_move() {
    std::string move;
    do {
        std::cout << ((board.side_to_move == 0) ? "White" : "Black") << " to move: ";
        if (!(std::cin >> move)) {  // exit on EOF (ctrl + z + enter on Windows *apparently*)
            std::cout << "Game ended prematurely by user." << std::endl;
            return "exit";
        }
    } while (!is_valid_move(move));

    return move;
}

bool is_valid_move(const std::string& move) {
    (void)move;
    // Placeholder implementation
    return true;
}

/**
 * @brief Modifies the internal board state based on the move provided.
 *
 * This function updates the board state, including the side to move, castling rights,
 * en passant square, half-move clock, and full move number.
 * 
 * The function also checks for checkmate and stalemate conditions after the move and ends the game
 * accordingly.
 *
 * @param move The move to be made in algebraic notation (e.g., "d2d4", or e7e8q for a promotion).
 * 
 * @return true if the game continues, false if the game ends (checkmate or stalemate).
 */
bool make_move(const std::string& move) {
    if (move.length() != 4 && move.length() != 5) return false;
    int from_index = alg_to_square_idx(move.substr(0, 2));
    int to_index = alg_to_square_idx(move.substr(2, 2));

    uint8_t dest_piece = board.squares[to_index];
    uint8_t moving_piece = board.squares[from_index];

    // 'Move' the piece in memory
    board.squares[to_index] = moving_piece;
    board.squares[from_index] = NONE;

    // Update bitboard for moving piece
    board.bitboards[piece_to_bitboard_idx(moving_piece)] |= 1ULL << to_index;
    board.bitboards[piece_to_bitboard_idx(moving_piece)] &= ~(1ULL << from_index);
    
    // If taking a piece, update its bitboard too
    if (dest_piece != NONE) {
        board.bitboards[piece_to_bitboard_idx(dest_piece)] &= ~(1ULL << to_index);
    }
    
    // Check for promotions
    if (move.length() == 5) {
        uint8_t promotion_piece;
        switch (move[5]) {
            case 'q': promotion_piece = QUEEN; break;
            case 'r': promotion_piece = ROOK; break;
            case 'b': promotion_piece = BISHOP; break;
            case 'n': promotion_piece = KNIGHT; break;
            default: promotion_piece = NONE;
        }
        promotion_piece |= board.side_to_move ? WHITE : BLACK;
        board.squares[to_index] = promotion_piece;
        board.bitboards[piece_to_bitboard_idx(promotion_piece)] |= 1ULL << to_index;
        // Remove placement on bitboard from original pawn
        board.bitboards[piece_to_bitboard_idx(moving_piece)] &= ~(1ULL << to_index);
    }

    return check_end_game_conditions();
}

bool check_end_game_conditions() {
    if (is_checkmate()) {
        print_board(true);
        std::cout << (board.side_to_move == 0 ? "White" : "Black") << " is checkmated. "
                << (board.side_to_move == 1 ? "White" : "Black") << " wins." << std::endl;
        return false;
    }
    
    if (is_stalemate()) {
        print_board(true);
        std::cout << "Stalemate!" << std::endl;
        return false;
    }

    return true;
}

bool is_checkmate() {
    // Placeholder implementation
    return false;
}

bool is_stalemate() {
    // Placeholder implementation
    return false;
}
