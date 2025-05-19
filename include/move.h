#ifndef MOVE_H
#define MOVE_H

#include <string>

// Move encoding: bits 0-5: from, bits 6-11: to, bits 12-15: flag
typedef uint16_t Move;

std::string get_next_player_move();
bool is_valid_move(const std::string& move);
bool make_move(const std::string& move);
bool is_checkmate();
bool is_stalemate();

#endif
