#include <iostream>
#include <cstdint>

#include "board.h"
#include "movegen.h" // advanced logic for generating the next move
#include "move.h" // actually performing the move inside the program
#include "search.h"
#include "eval.h"
#include "opening.h"
#include "util.h"

void play();

int main() {
    std::cout << "Chess engine starting..." << std::endl;

    // Get if player1/2 is human/bot
    get_player_types();

    // Board setup
    init_board();

    // Start game loop
    play();

    return 0;
}

void play() {
    while (true) {
        print_board(true, true);
        std::string move;
        move = player_types[board.side_to_move] ? get_next_player_move() : get_next_bot_move();
        if (move == "exit") break;
        if (!make_move(move)) break;

        board.side_to_move ^= 1; // switch player: 0 -> 1, 1 -> 0
    }
}
