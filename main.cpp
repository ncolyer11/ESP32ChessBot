#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <thread>

#include "movegen.h" // advanced logic for generating all moves and picking the best one
#include "util.h"

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
        std::string move;
        move = player_types[board.side_to_move] ? get_next_player_move() : get_next_bot_move();
        if (move == "exit") break;
        if (!make_move(move)) break;

        board.side_to_move ^= 1; // switch player: 0 -> 1, 1 -> 0
        // std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
