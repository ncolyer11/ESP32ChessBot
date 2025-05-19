#ifndef FEN
#define FEN

#include <string>

void load_position_from_fen(const char *fen);
std::string export_position_to_fen();

#endif