#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <thread>

#include "movegen.h" // advanced logic for generating all moves and picking the best one
#include "util.h"

// https://lichess.org/editor

void play();

int main() {
    // Seed rng for picking a random move (and later a random gm opening move)
    std::srand(std::time(nullptr));

    std::cout << "Chess engine starting..." << std::endl;

    // Get if player1/2 is human/bot
    get_player_types();

    // Boards setup
    init_board();
    init_attack_bitboards();

    // Start game loop
    play();

    return 0;
}

void play() {
    while (true) {
        print_board(true, false); // show params, show bitboards
        Move move = player_types[board.side_to_move] ? get_next_player_move() : get_next_bot_move();
        // exit on error or ctrl + d
        if (move == INVALID_MOVE) break;
        // exit on end of game and print checkmate or stalemate message
        if (move == NO_MOVES) { check_end_game_conditions(&board, legal_move_count); break; }
        // exit on invalid move, also switches players
        if (!make_move_on_board(&board, move)) break;
        if (!player1_is_human && !player2_is_human) {
            // Pause for a bit so bots don't spam
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
