#include <chrono>
#include <conio.h>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <thread>
#include <windows.h>

#include "movegen.h" // advanced logic for generating all moves and picking the best one
#include "fen.h"
#include "util.h"

// https://lichess.org/editor

void play();
void check_for_test_mode();

int main() {
    check_for_test_mode();

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
        print_board(true, MODE || false); // show params, show bitboards or testing

        // Return if either king is missing
        if (!board.bitboards[0] || !board.bitboards[6]) return;

        Move move = player_types[board.side_to_move] ? get_next_player_move() : get_next_bot_move();

        // Exit on error or ctrl + d
        if (move == INVALID_MOVE) break;

        // Exit on end of game and print checkmate or stalemate message
        if (move == NO_MOVES) {
            check_end_game_conditions(&board, legal_move_count);
            break;
        }

        // Exit on invalid move, also switches players
        if (!make_move_on_board(&board, move)) break;

        // if (!player1_is_human && !player2_is_human) {
        //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // }
    }
    std::cout << "Export final position to FEN? (Y/n): ";
    std::string input;
    std::getline(std::cin, input);
    if (input.empty() || tolower(input[0]) == 'y') {
        std::cout << "FEN: " << export_position_to_fen() << std::endl;
    }
}

void check_for_test_mode() {
    int elapsed = 0;
    const int step = 5; // ms
    while (elapsed < 5000) {
        if (_kbhit()) {
            std::string input;
            std::getline(std::cin, input);
            if (input == "test") {
                MODE = true;
                std::cout << "TEST MODE ACTIVATED\n";
            }
            break;
        }
        Sleep(step);
        elapsed += step;
    }
}
