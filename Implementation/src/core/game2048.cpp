#include "game2048.h"
#include "../ai/heuristics.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstring>

Game2048::RowResult Game2048::row_table[65536];
bool Game2048::row_table_ready = false;

void Game2048::build_row_table() {
    if (row_table_ready) return;
    for (int row = 0; row < 65536; row++) {
        int t[4];
        t[0] = (row >>  0) & 0xF;
        t[1] = (row >>  4) & 0xF;
        t[2] = (row >>  8) & 0xF;
        t[3] = (row >> 12) & 0xF;

        int buf[4] = {};  // compact nonzero tiles to the left
        int idx = 0;
        for (int i = 0; i < 4; i++)
            if (t[i]) buf[idx++] = t[i];

        int res[4] = {};
        int ri = 0, pts = 0;
        for (int i = 0; i < idx; ) {
            if (i + 1 < idx && buf[i] == buf[i+1]) {
                int merged = buf[i] + 1;
                res[ri++] = merged;
                pts += (1 << merged);  // actual tile value scored
                i += 2;
            } else {
                res[ri++] = buf[i++];
            }
        }

        uint16_t packed = (uint16_t)(res[0] | (res[1] << 4) | (res[2] << 8) | (res[3] << 12));
        row_table[row] = {packed, pts};
    }
    row_table_ready = true;
}

uint16_t Game2048::extract_row(uint64_t board, int r) {
    return (uint16_t)((board >> (r * 16)) & 0xFFFF);
}

uint64_t Game2048::place_row(uint64_t board, int r, uint16_t row) {
    int shift = r * 16;
    return (board & ~(uint64_t(0xFFFF) << shift)) | (uint64_t(row) << shift);
}

uint64_t Game2048::rotate_cw(uint64_t b) {
    uint64_t res = 0;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            int rank = (b >> ((r * 4 + c) * 4)) & 0xF;
            int ni   = c * 4 + (3 - r);
            res |= uint64_t(rank) << (ni * 4);
        }
    return res;
}

uint64_t Game2048::rotate_ccw(uint64_t b) {
    uint64_t res = 0;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            int rank = (b >> ((r * 4 + c) * 4)) & 0xF;
            int ni   = (3 - c) * 4 + r;
            res |= uint64_t(rank) << (ni * 4);
        }
    return res;
}

// mirror each row left-right
uint64_t Game2048::mirror_rows(uint64_t b) {
    uint64_t res = 0;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            int rank = (b >> ((r * 4 + c) * 4)) & 0xF;
            int ni   = r * 4 + (3 - c);
            res |= uint64_t(rank) << (ni * 4);
        }
    return res;
}

Game2048::Game2048(bool spawn_tiles) : board(0), score(0), highest_tile(0), move_count(0) {
    build_row_table();
    if (spawn_tiles) {
        spawn_tile();
        spawn_tile();
    }
}

void Game2048::decode(int out[4][4]) const {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            int rank = (board >> ((r * 4 + c) * 4)) & 0xF;
            out[r][c] = rank ? (1 << rank) : 0;
        }
}
std::vector<std::pair<int,int>> Game2048::get_empty_cells() const {
    std::vector<std::pair<int,int>> cells;
    cells.reserve(16);
    for (int i = 0; i < 16; i++)
        if (((board >> (i * 4)) & 0xF) == 0)
            cells.push_back({i / 4, i % 4});
    return cells;
}
int Game2048::count_empty_cells() const {
    int count = 0;
    for (int i = 0; i < 16; i++)
        if (((board >> (i * 4)) & 0xF) == 0) count++;
    return count;
}
int Game2048::get_max_tile() const {
    int max_rank = 0;
    for (int i = 0; i < 16; i++) {
        int rank = (board >> (i * 4)) & 0xF;
        if (rank > max_rank) max_rank = rank;
    }
    return max_rank ? (1 << max_rank) : 0;
}
bool Game2048::spawn_tile() {
    auto empty = get_empty_cells();
    if (empty.empty()) return false;

    auto& [r, c] = empty[rand_int(0, static_cast<int>(empty.size() - 1))];
    int rank = (rand_double() < 0.9) ? 1 : 2;   // rank 1 = tile 2, rank 2 = tile 4
    board = set_tile(board, r * 4 + c, rank);
    update_highest();
    return true;
}
bool Game2048::spawn_evil_tile() {
    auto empty = get_empty_cells();
    if (empty.empty()) return false;

    double worst_score = 1e18;
    int worst_idx = empty[0].first * 4 + empty[0].second;
    int worst_rank = 1;

    int decoded[4][4];
    decode(decoded);

    for (auto& [r, c] : empty) {
        for (auto& [rank, prob] : {std::pair{1, 0.9}, std::pair{2, 0.1}}) {
            int saved = decoded[r][c];
            decoded[r][c] = (1 << rank);
            double s = Heuristics::evaluate(decoded);
            if (s < worst_score) {
                worst_score = s;
                worst_idx  = r * 4 + c;
                worst_rank = rank;
            }
            decoded[r][c] = saved;
        }
    }
    board = set_tile(board, worst_idx, worst_rank);
    update_highest();
    return true;
}

Game2048 Game2048::clone() const {
    Game2048 copy(false);
    copy.board        = board;
    copy.score        = score;
    copy.highest_tile = highest_tile;
    copy.move_count   = move_count;
    return copy;
}
void Game2048::update_highest() {
    int mv = get_max_tile();
    if (mv > highest_tile) highest_tile = mv;
}
static uint64_t slide_left(uint64_t b, int& total_pts, const Game2048::RowResult* table) {
    for (int r = 0; r < 4; r++) {
        uint16_t row = Game2048::extract_row(b, r);
        auto& res = table[row];
        b = Game2048::place_row(b, r, res.row);
        total_pts += res.score;
    }
    return b;
}
bool Game2048::make_move(Direction dir) {
    uint64_t working = board;
    switch (dir) {
        case Direction::RIGHT:  working = mirror_rows(working);  break;
        case Direction::UP:     working = rotate_ccw(working);   break;
        case Direction::DOWN:   working = rotate_cw(working);    break;
        default: break;
    }

    int pts = 0;
    uint64_t moved = slide_left(working, pts, row_table);

    if (moved == working) return false;   // no change

    switch (dir) {
        case Direction::RIGHT:  moved = mirror_rows(moved);  break;
        case Direction::UP:     moved = rotate_cw(moved);    break;
        case Direction::DOWN:   moved = rotate_ccw(moved);   break;
        default: break;
    }

    board = moved;
    score += pts;
    move_count++;
    update_highest();
    return true;
}
std::pair<Game2048, bool> Game2048::simulate_move(Direction dir) const {
    Game2048 copy = clone();
    bool valid = copy.make_move(dir);
    return {copy, valid};
}

bool Game2048::can_move(Direction dir) const {
    uint64_t working = board;
    switch (dir) {
        case Direction::RIGHT:  working = mirror_rows(working);  break;
        case Direction::UP:     working = rotate_ccw(working);   break;
        case Direction::DOWN:   working = rotate_cw(working);    break;
        default: break;
    }
    int dummy = 0;
    uint64_t moved = slide_left(working, dummy, row_table);
    return moved != working;
}

std::vector<Direction> Game2048::get_valid_moves() const {
    std::vector<Direction> valid;
    valid.reserve(4);
    for (auto dir : ALL_DIRECTIONS)
        if (can_move(dir)) valid.push_back(dir);
    return valid;
}

bool Game2048::is_game_over() const {
    for (auto dir : ALL_DIRECTIONS)
        if (can_move(dir)) return false;
    return true;
}

bool Game2048::has_won() const {
    return highest_tile >= 2048;
}

void Game2048::print_board() const {
    int decoded[4][4];
    decode(decoded);
    std::cout << "Score: " << score
              << "  |  Highest: " << highest_tile
              << "  |  Moves: " << move_count << "\n";
    std::cout << "+------+------+------+------+\n";
    for (int r = 0; r < 4; r++) {
        std::cout << "|";
        for (int c = 0; c < 4; c++) {
            if (decoded[r][c] == 0)
                std::cout << "   .  |";
            else
                std::cout << std::setw(5) << decoded[r][c] << " |";
        }
        std::cout << "\n+------+------+------+------+\n";
    }
}
