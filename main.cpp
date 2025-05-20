#include <chrono>
#include <conio.h>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <iostream>
#include <thread>
#include <windows.h>

#include "movegen.h" // advanced logic for generating all moves and picking the best one
#include "util.h"

// https://lichess.org/editor

void play();
void check_for_test_mode();

std::deque<Board> board_history;

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
        // Reset board and history for each new game
        init_board();
        board_history.clear();
        board_history.push_back(board);

        while (true) {
            print_board(true, false); // show params, show bitboards

            // Store up to last 5 positions
            if (board_history.size() == 5) board_history.pop_front();
            board_history.push_back(board);

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

        // Print last 5 board states
        std::cout << "\n=== Last 5 Board States ===\n";
        for (const Board& past : board_history) {
            board = past;
            print_board(true, false);
            std::cout << "------------------------\n";
        }
        std::cout << "=== Starting new game ===\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void check_for_test_mode() {
    int elapsed = 0;
    const int step = 5; // ms
    while (elapsed < 50) {
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
