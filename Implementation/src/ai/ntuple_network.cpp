#include "ntuple_network.h"

#include <cstdio>
#include <cstring>
#include <fstream>

namespace {

// The 8 symmetries of the square, as maps on (row, col) of the 4x4 board.
// Together these are the dihedral group D4: 4 rotations and 4 reflections.
void transform(int sym, int r, int c, int& out_r, int& out_c) {
    switch (sym) {
        case 0: out_r = r;         out_c = c;         break;  // identity
        case 1: out_r = c;         out_c = 3 - r;     break;  // rotate 90 cw
        case 2: out_r = 3 - r;     out_c = 3 - c;     break;  // rotate 180
        case 3: out_r = 3 - c;     out_c = r;         break;  // rotate 270 cw
        case 4: out_r = r;         out_c = 3 - c;     break;  // mirror columns
        case 5: out_r = 3 - r;     out_c = c;         break;  // mirror rows
        case 6: out_r = c;         out_c = r;         break;  // transpose
        default: out_r = 3 - c;    out_c = 3 - r;     break;  // anti-transpose
    }
}

// Packs the 4-bit tile ranks of `cells` into one table index.
inline uint32_t tuple_index(uint64_t board, const int* cells, int n) {
    uint32_t idx = 0;
    for (int k = 0; k < n; ++k) {
        idx |= uint32_t(get_tile(board, cells[k])) << (4 * k);
    }
    return idx;
}

// Rows, columns and 2x2 blocks with extensions, biased towards the top-left
// corner; the 8 symmetries cover the rest of the board.  Cell index = r*4 + c.
const std::vector<std::vector<int>>& default_patterns() {
    static const std::vector<std::vector<int>> kPatterns = {
        {0, 1, 2, 3, 4},      // top row + one below
        {4, 5, 6, 7, 8},      // second row + one below
        {0, 1, 2, 4, 5},      // top-left block, wide
        {4, 5, 6, 8, 9},      // middle block, wide
        {0, 1, 4, 5, 8},      // left column pair + extension
        {1, 2, 5, 6, 9},      // inner column pair + extension
        {0, 1, 2, 5, 6},      // top row + inner pair
        {1, 5, 6, 9, 10},     // centre block + extension
    };
    return kPatterns;
}

const char kMagic[8] = {'N', 'T', '2', '0', '4', '8', '\0', '\0'};
constexpr uint32_t kVersion = 1;

}  // namespace

void NTupleNetwork::build_symmetries(Pattern& p) {
    p.length = static_cast<int>(p.base_cells.size());
    for (int s = 0; s < kSymmetries; ++s) {
        for (int k = 0; k < p.length; ++k) {
            const int cell = p.base_cells[k];
            int out_r = 0, out_c = 0;
            transform(s, cell / 4, cell % 4, out_r, out_c);
            p.cells[s][k] = out_r * 4 + out_c;
        }
    }
}

NTupleNetwork::NTupleNetwork() {
    const auto& base = default_patterns();
    _patterns.resize(base.size());
    for (std::size_t i = 0; i < base.size(); ++i) {
        Pattern& p = _patterns[i];
        p.base_cells = base[i];
        build_symmetries(p);
        // 16 possible ranks per cell (0 = empty, 15 = the 32768 tile)
        p.weights.assign(std::size_t(1) << (4 * p.length), 0.0f);
    }
    _touched_per_board = static_cast<int>(_patterns.size()) * kSymmetries;
}

double NTupleNetwork::value(uint64_t board) const {
    double sum = 0.0;
    for (const Pattern& p : _patterns) {
        for (int s = 0; s < kSymmetries; ++s) {
            sum += p.weights[tuple_index(board, p.cells[s], p.length)];
        }
    }
    return sum;
}

void NTupleNetwork::update(uint64_t board, double delta) {
    // Spread the correction evenly, so value(board) shifts by ~delta rather
    // than by delta * (number of weights).
    const float per_weight = static_cast<float>(delta / _touched_per_board);
    for (Pattern& p : _patterns) {
        for (int s = 0; s < kSymmetries; ++s) {
            p.weights[tuple_index(board, p.cells[s], p.length)] += per_weight;
        }
    }
}

std::size_t NTupleNetwork::total_weights() const {
    std::size_t n = 0;
    for (const Pattern& p : _patterns) n += p.weights.size();
    return n;
}

double NTupleNetwork::memory_mb() const {
    return double(total_weights() * sizeof(float)) / (1024.0 * 1024.0);
}

bool NTupleNetwork::save(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    f.write(kMagic, sizeof(kMagic));
    const uint32_t version = kVersion;
    f.write(reinterpret_cast<const char*>(&version), sizeof(version));

    const uint32_t count = static_cast<uint32_t>(_patterns.size());
    f.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const Pattern& p : _patterns) {
        const uint32_t len = static_cast<uint32_t>(p.length);
        f.write(reinterpret_cast<const char*>(&len), sizeof(len));
        for (int cell : p.base_cells) {
            const int32_t c = cell;
            f.write(reinterpret_cast<const char*>(&c), sizeof(c));
        }
    }

    f.write(reinterpret_cast<const char*>(&games_trained), sizeof(games_trained));

    for (const Pattern& p : _patterns) {
        f.write(reinterpret_cast<const char*>(p.weights.data()),
                std::streamsize(p.weights.size() * sizeof(float)));
    }
    return f.good();
}

bool NTupleNetwork::load(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    char magic[sizeof(kMagic)] = {};
    f.read(magic, sizeof(magic));
    if (std::memcmp(magic, kMagic, sizeof(kMagic)) != 0) return false;

    uint32_t version = 0;
    f.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != kVersion) return false;

    uint32_t count = 0;
    f.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (count != _patterns.size()) return false;   // different pattern set

    for (const Pattern& p : _patterns) {
        uint32_t len = 0;
        f.read(reinterpret_cast<char*>(&len), sizeof(len));
        if (int(len) != p.length) return false;
        for (int k = 0; k < p.length; ++k) {
            int32_t cell = 0;
            f.read(reinterpret_cast<char*>(&cell), sizeof(cell));
            if (cell != p.base_cells[std::size_t(k)]) return false;
        }
    }

    long long games = 0;
    f.read(reinterpret_cast<char*>(&games), sizeof(games));

    for (Pattern& p : _patterns) {
        f.read(reinterpret_cast<char*>(p.weights.data()),
               std::streamsize(p.weights.size() * sizeof(float)));
    }
    if (!f.good()) return false;

    games_trained = games;
    return true;
}
