#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>

#include "chess_search.h"
#include "eval.h"
#include "fen.h"
#include "movegen.h"
#include "util.h"

// https://lichess.org/editor
// chcp 65001 to fix unicode in cmd

enum class Mode { ASK, HVB, BVH, HVH, BVB };

struct PerftCase { const char *fen; int depth; uint64_t expected[5]; };

void play(Board *b);
void init_all(Board *b, const std::string &fen);
void print_usage();
int run_perft(const std::string &fen, int depth);
int run_perft_suite();
void run_bench(int depth, int time_ms);
bool parse_mode(const char *s, Mode *mode);
void set_players(Mode mode);
Mode ask_mode();

int main(int argc, char **argv) {
    Mode mode = Mode::ASK;
    std::string fen;
    int perft_depth = 0;
    bool bench = false, perft_suite = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        bool has_value = i + 1 < argc;
        if (arg == "--help" || arg == "-h") {
            print_usage();
            return 0;
        } else if (arg == "--test") {
            MODE = true;
        } else if (arg == "--bench") {
            bench = true;
        } else if (arg == "--perft-suite") {
            perft_suite = true;
        } else if (arg == "--mode" && has_value) {
            if (!parse_mode(argv[++i], &mode)) {
                std::cerr << "Unknown mode: " << argv[i] << " (use hvb, bvh, hvh or bvb)" << std::endl;
                return 2;
            }
        } else if (arg == "--depth" && has_value) {
            bot_depth = std::atoi(argv[++i]);
            if (bot_depth < 1) bot_depth = 1;
        } else if (arg == "--time" && has_value) {
            bot_time_ms = std::atoi(argv[++i]);
            if (bot_time_ms < 0) bot_time_ms = 0;
        } else if (arg == "--fen" && has_value) {
            fen = argv[++i];
        } else if (arg == "--perft" && has_value) {
            perft_depth = std::atoi(argv[++i]);
            if (perft_depth < 1) perft_depth = 1;
        } else {
            std::cerr << "Unknown or incomplete option: " << arg << "\n" << std::endl;
            print_usage();
            return 2;
        }
    }

    // Seed rng for picking opening book moves (kept deterministic in test mode)
    if (!MODE) std::srand(std::time(nullptr));

    if (perft_suite) {
        init_attack_bitboards();
        return run_perft_suite();
    }
    if (perft_depth > 0) {
        init_attack_bitboards();
        return run_perft(fen.empty() ? starting_fen : fen, perft_depth);
    }

    std::cout << "Chess engine starting..." << std::endl;

    Board b;
    if (MODE) get_player_types(); // test mode asks for players first, then the FEN
    init_all(&b, fen);

    if (bench) {
        run_bench(bot_depth, bot_time_ms);
        return 0;
    }

    // Players: test mode already asked above
    if (!MODE) {
        if (mode == Mode::ASK) mode = ask_mode();
        set_players(mode);
    }

    // Start game loop
    play(&b);

    return 0;
}

void init_all(Board *b, const std::string &fen) {
    init_attack_bitboards();
    init_distance_tables();
    init_board(b, fen);
}

void print_usage() {
    std::cout <<
        "Usage: chesp32 [options]\n"
        "  --mode hvb|bvh|hvh|bvb  who plays: human/bot per side (default: ask, then human white vs bot)\n"
        "  --depth N               bot max search depth (default " << DEFAULT_DEPTH << ")\n"
        "  --time MS               bot time limit per move in ms (default 0 = no limit)\n"
        "  --fen \"<fen>\"           start from this position\n"
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

Mode ask_mode() {
    std::cout << "Play as (w)hite, (b)lack, (2) players or (0) bots-only? [w] " << std::flush;
    std::string input;
    if (!std::getline(std::cin, input)) return Mode::HVB;
    normalise_line(input);
    if (input.empty()) return Mode::HVB;
    switch (tolower(input[0])) {
        case 'b': return Mode::BVH;
        case '2': return Mode::HVH;
        case '0': return Mode::BVB;
        default: return Mode::HVB;
    }
}

void set_players(Mode mode) {
    player_types[0] = (mode == Mode::HVB || mode == Mode::HVH);
    player_types[1] = (mode == Mode::BVH || mode == Mode::HVH);
    player1_is_human = player_types[0];
    player2_is_human = player_types[1];
}

void play(Board *b) {
    while (true) {
        print_board(b, true, MODE || false); // show params, show bitboards or testing

        // Return if either king is missing
        if (!b->bitboards[0] || !b->bitboards[6]) return;

        Move move;
        if (player_types[b->side_to_move]) {
            move = get_next_player_move(b);
        } else {
            // game-over check before the bot starts thinking
            MoveList legal;
            if (gen_and_validate_all_moves(b, &legal) == NO_MOVES) break;
            move = get_next_bot_move(b, bot_depth, bot_time_ms);
        }

        // An undo happened: loop again so the right kind of player is picked for the side to move
        if (move == UNDO_DONE) continue;

        // Exit on error or ctrl + d
        if (move == INVALID_MOVE) break;

        // Exit on end of game
        if (move == NO_MOVES) break;

        // Exit on invalid move
        if (!make_move_on_board(b, move)) break;
    }
    ask_export_fen(b);
}

// ---------------------------------------------------------------------------
// perft / bench
// ---------------------------------------------------------------------------
int run_perft(const std::string &fen, int depth) {
    Board b;
    if (!load_position_from_fen(&b, fen)) return 1;
    uint64_t total = 0;
    auto start = std::chrono::steady_clock::now();
    for (int d = 1; d <= depth; d++) {
        uint64_t nodes = perft(&b, d);
        total = nodes;
        std::cout << "perft(" << d << ") = " << nodes << std::endl;
    }
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::cout << "time " << (long long)ms << " ms, " << (ms > 0 ? (long long)(total / ms) : 0) << " nodes/ms at depth " << depth << std::endl;
    return 0;
}

int run_perft_suite() {
    static const PerftCase cases[] = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4, {20, 400, 8902, 197281, 0}},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, {48, 2039, 97862, 4085603, 0}},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5, {14, 191, 2812, 43238, 674624}},
        {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, {6, 264, 9467, 422333, 0}},
    };
    int failures = 0;
    for (const PerftCase &c : cases) {
        Board b;
        load_position_from_fen(&b, c.fen);
        for (int d = 1; d <= c.depth; d++) {
            uint64_t nodes = perft(&b, d);
            bool ok = nodes == c.expected[d - 1];
            std::cout << (ok ? "ok   " : "FAIL ") << "d" << d << " " << nodes;
            if (!ok) std::cout << " (expected " << c.expected[d - 1] << ")";
            std::cout << "  " << c.fen << std::endl;
            if (!ok) failures++;
        }
    }
    std::cout << (failures ? "perft FAILED" : "perft passed") << std::endl;
    return failures ? 1 : 0;
}

void run_bench(int depth, int time_ms) {
    static const char *fens[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "r1bq1rk1/pp2ppbp/2np1np1/8/3NP3/2N1BP2/PPPQ2PP/R3KB1R w KQ - 0 9",
        "8/8/4k3/8/2KP4/8/8/8 w - - 0 1",
    };
    double total = 0;
    for (const char *fen : fens) {
        Board b;
        load_position_from_fen(&b, fen);
        auto start = std::chrono::steady_clock::now();
        get_next_bot_move(&b, depth, time_ms);
        double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        total += ms;
        std::cout << "bench depth " << depth << ": " << (long long)ms << " ms  " << fen << std::endl;
    }
    std::cout << "bench total: " << (long long)total << " ms" << std::endl;
}
