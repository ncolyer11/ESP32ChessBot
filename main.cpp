#include <chrono>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <thread>

#include "movegen.h" // advanced logic for generating all moves and picking the best one
#include "fen.h"
#include "chess_search.h"
#include "eval.h"
#include "util.h"

// https://lichess.org/editor
// chcp 65001 to fix unicode in cmd
/** TODO: 
 * - investigate pawn not being able to take bishop pinning it
 * I think this is caused by legal_move_list not being updated
 */

constexpr int DEPTH = 6; // depth for the bot to search

void play(Board *b);
// void print_all_opening_moves();

int main() {
    test_fens();
    return 0;
    // print_all_opening_moves();
    check_for_test_mode();
    
    // Seed rng for picking a random move (and later a random gm opening move)
    std::srand(std::time(nullptr));
    
    std::cout << "Chess engine starting..." << std::endl;
    
    // Get if player1/2 is human/bot
    get_player_types();
    
    // Boards setup
    Board b;
    init_board(&b);
    init_attack_bitboards();

    // Start game loop
    play(&b);

    return 0;
}

void play(Board *b) {
    while (true) {
        print_board(b, true, MODE || false); // show params, show bitboards or testing

        // Return if either king is missing
        if (!b->bitboards[0] || !b->bitboards[6]) return;

        Move move = player_types[b->side_to_move] ? get_next_player_move(b) 
                                                  : get_next_bot_move(b, DEPTH);

        // Exit on error or ctrl + d
        if (move == INVALID_MOVE) break;

        // Exit on end of game
        if (move == NO_MOVES) break;

        // Exit on invalid move, also switches players
        move_history.push_back(move);
        if (!make_move_on_board(b, move)) break;

        // if (!player1_is_human && !player2_is_human) {
        //     std::this_thread::sleep_for(std::chrono::milliseconds(500));
        // }
    }
    ask_export_fen(b);
}

// void print_all_opening_moves() {
//     const char* move_number_names[3] = {"First", "Second", "Third"};
//     const char* color_names[2] = {"White", "Black"};
//     for (int move_num = 0; move_num < 3; ++move_num) {
//         for (int color = 0; color < 2; ++color) {
//             std::cout << move_number_names[move_num] << " move options for " << color_names[color] << ":\n";
//             for (int i = 0; i < 10; ++i) {
//                 Move move = opening_moves_encoded[move_num][color][i];
//                 std::cout << "  " << move_to_move_str(move) << " (0x" << std::hex << move << std::dec << ")\n";
//             }
//         }
//     }
// }

void test_fens() {
    std::vector<std::string> fens = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", // Initial position
        "rnbqkbnr/pp1ppppp/8/2p5/8/5N2/PPPPPPPP/RNBQKB1R w KQkq c6 0 2", // After 1. Nf3 c5
        "r1bqkbnr/pppp1ppp/2n5/4p3/1bP5/2N2N2/PP1PPPPP/R1BQKB1R w KQkq - 2 4", // Sicilian defense
        "8/8/8/8/8/8/PPP5/K1k5 w - - 0 1", // Endgame position
        "rnb1k1nr/pppp1ppp/8/4p3/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 2 3", // Italian game
        "8/5k2/8/8/8/8/4K3/8 b - - 0 1", // King vs king
        "rnbq1bnr/pppp1ppp/4pk2/8/3P4/8/PPP2PPP/RNBQKBNR w KQ - 1 4", // Early f3 blunder
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 10" // GM-level opening complexity
    };

    for (const auto &fen : fens) {
        Board b;
        load_position_from_fen(&b, fen);
        std::cout << "FEN: " << fen << "\n";
        int score = evaluate(&b);
        std::cout << "Evaluation: " << score << "\n\n";
    }
}
