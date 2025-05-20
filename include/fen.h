#pragma once

#include <string>
#include <regex>

void load_position_from_fen(std::string);
std::string export_position_to_fen();
std::string choose_fen();


static const std::string starting_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
static const std::string check_fen = "4k3/8/2p5/2P5/8/2b5/1N3P2/K6N w - - 0 1";
static const std::string stalemate_fen = "7k/2p5/5N2/2P5/8/5R2/1N3P2/K7 b - - 0 1";
static const std::string queens_fen = "rnbqkbnr/qpqqpqpp/7p/8/8/P7/PQQPPQQQ/RNBQKBNR w KQkq - 0 1";
static const std::string coverage_fen = "r3kb1r/pp3bP1/3n4/1r6/1n1p4/R7/pKPQ1p1q/8 w q - 0 1";
static const std::string castling_fen = "r3k1r1/1ppppp2/3r1p2/8/8/4PP2/1PP1PP2/R3K2R w KQkq - 0 1";

// Basic FEN regex: 6 space-separated fields, first field = at least 8 chars (for pieces/ranks)
static const std::regex fen_regex(
    R"(([rnbqkpRNBQKP1-8]+\/){7}[rnbqkpRNBQKP1-8]+ [wb] (K?Q?k?q?|-) (-|[a-h][36]) \d+ \d+)"
);
