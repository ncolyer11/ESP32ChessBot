#pragma once

#include <string>

#include "board.h"

extern const std::string starting_fen;

// Returns false (and loads the starting position) if the FEN is invalid
bool load_position_from_fen(Board *b, std::string fen);
bool is_valid_fen(const std::string &fen);
std::string export_position_to_fen(Board *b);
std::string choose_fen();
void ask_export_fen(Board *b);
