#include <chrono>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <thread>

#include "chess_search.h"
#include "eval.h"
#include "fen.h"
#include "movegen.h"
#include "util.h"

// https://lichess.org/editor
// chcp 65001 to fix unicode in cmd

/** TODO: 
 * - investigate pawn not being able to take bishop pinning it
 * I think this is caused by legal_move_list not being updated
 * ADD BACK IN THE MOVE REP STUFF NOW THAT YOUVE DEBUGGED HELL :))
 */

void play(Board *b);
void init_all(Board *b);


int main() {
    check_for_test_mode();
    
    // Seed rng for picking a random move (and later a random gm opening move)
    std::srand(std::time(nullptr));
    
    std::cout << "Chess engine starting..." << std::endl;
    
    // Boards setup
    Board b;
    init_all(&b);
    
    // Get if player1/2 is human/bot
    get_player_types();
    
    // Start game loop
    play(&b);

    return 0;
}

void init_all(Board *b) {
    init_board(b);
    init_attack_bitboards();
    init_distance_tables();
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
    }
    ask_export_fen(b);
}
