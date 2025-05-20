#pragma once

#include <string>

void load_position_from_fen(const char *fen);
std::string export_position_to_fen();
const char *choose_fen();


static constexpr char starting_fen[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
static constexpr char check_fen[] = "4k3/8/2p5/2P5/8/2b5/1N3P2/K6N w - - 0 1";
static constexpr char stalemate_fen[] = "7k/2p5/5N2/2P5/8/5R2/1N3P2/K7 b - - 0 1";
static constexpr char queens_fen[] = "rnbqkbnr/qpqqpqpp/7p/8/8/P7/PQQPPQQQ/RNBQKBNR w KQkq - 0 1";
static constexpr char coverage_fen[] = "r3kb1r/pp3bP1/3n4/1r6/1n1p4/R7/pKPQ2qq/8 w q - 0 1";
