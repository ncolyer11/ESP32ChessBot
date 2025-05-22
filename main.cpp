#include <chrono>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <thread>

#include "movegen.h" // advanced logic for generating all moves and picking the best one
#include "fen.h"
#include "chess_search.h"
#include "util.h"

// https://lichess.org/editor
// chcp 65001 to fix unicode in cmd

constexpr int DEPTH = 6; // depth for the bot to search

void play(Board *b);

int main() {
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
        if (!make_move_on_board(b, move)) break;

        // if (!player1_is_human && !player2_is_human) {
        //     std::this_thread::sleep_for(std::chrono::milliseconds(500));
        // }
    }
    ask_export_fen(b);
}
