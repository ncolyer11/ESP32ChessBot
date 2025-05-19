#ifndef MOVE_H
#define MOVE_H

#include <string>

std::string get_next_player_move();
bool is_valid_move(const std::string& move);
bool make_move(const std::string& move);
bool is_checkmate();
bool is_stalemate();

#endif
