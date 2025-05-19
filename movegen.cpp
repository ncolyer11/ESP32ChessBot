#include <iostream>

#include "movegen.h"
#include "board.h"

// Generates the next move using an advanced algorithm
std::string get_next_bot_move() {
    std::string colour_to_move = (board.side_to_move == 0 ? "White" : "Black");

    // Placeholder implementation
    std::string move = "e2e4";
    std::cout << "Chesp32 (" << colour_to_move << ") plays: " << move << std::endl;
    return move;
}

// Generates all possible moves for the current position
int generate_moves() {
    // Minimal placeholder implementation
    return 20; // Example value
}
