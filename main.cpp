#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "app.h"

// https://lichess.org/editor
// chcp 65001 to fix unicode in cmd

void print_usage();
bool parse_mode(const char *s, Mode *mode);

int main(int argc, char **argv) {
    AppConfig cfg;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        bool has_value = i + 1 < argc;
        if (arg == "--help" || arg == "-h") {
            print_usage();
            return 0;
        } else if (arg == "--test") {
            cfg.test = true;
        } else if (arg == "--ascii") {
            cfg.ascii_board = true;
        } else if (arg == "--bench") {
            cfg.bench = true;
        } else if (arg == "--perft-suite") {
            cfg.perft_suite = true;
        } else if (arg == "--mode" && has_value) {
            if (!parse_mode(argv[++i], &cfg.mode)) {
                std::cerr << "Unknown mode: " << argv[i] << " (use hvb, bvh, hvh or bvb)" << std::endl;
                return 2;
            }
        } else if (arg == "--depth" && has_value) {
            cfg.depth = std::atoi(argv[++i]);
            if (cfg.depth < 1) cfg.depth = 1;
        } else if (arg == "--time" && has_value) {
            cfg.time_ms = std::atoi(argv[++i]);
            if (cfg.time_ms < 0) cfg.time_ms = 0;
        } else if (arg == "--fen" && has_value) {
            cfg.fen = argv[++i];
        } else if (arg == "--perft" && has_value) {
            cfg.perft_depth = std::atoi(argv[++i]);
            if (cfg.perft_depth < 1) cfg.perft_depth = 1;
        } else {
            std::cerr << "Unknown or incomplete option: " << arg << "\n" << std::endl;
            print_usage();
            return 2;
        }
    }

    app_init();
    return app_run(cfg);
}

void print_usage() {
    std::cout <<
        "Usage: chesp32 [options]\n"
        "  --mode hvb|bvh|hvh|bvb  who plays: human/bot per side (default: ask, then human white vs bot)\n"
        "  --depth N               bot max search depth (default " << DEFAULT_DEPTH << ")\n"
        "  --time MS               bot time limit per move in ms (default 0 = no limit)\n"
        "  --fen \"<fen>\"           start from this position\n"
        "  --ascii                 plain letters for the pieces (for terminals without unicode)\n"
        "  --test                  deterministic test mode (ascii board, FEN + player prompts on stdin)\n"
        "  --perft N               count legal move tree leaves to depth N (with optional --fen)\n"
        "  --perft-suite           run the known perft numbers, non-zero exit on mismatch\n"
        "  --bench                 time the bot on a few positions at --depth\n"
        "  --help                  show this help\n"
        "\n"
        "In game type a move like e2e4 (e7e8q to promote, default queen) or a command:\n"
        "  u undo to your last turn, z undo 2 plies, f print FEN, d bitboard board,\n"
        "  p params, b bitboards, r resign, q quit, h help\n";
}

bool parse_mode(const char *s, Mode *mode) {
    if (!std::strcmp(s, "hvb")) *mode = Mode::HVB;
    else if (!std::strcmp(s, "bvh")) *mode = Mode::BVH;
    else if (!std::strcmp(s, "hvh")) *mode = Mode::HVH;
    else if (!std::strcmp(s, "bvb")) *mode = Mode::BVB;
    else return false;
    return true;
}
