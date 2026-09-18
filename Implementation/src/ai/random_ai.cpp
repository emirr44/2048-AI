#include "random_ai.h"

Direction RandomAI::get_move(const Game2048& game) {
    auto valid = game.get_valid_moves();
    return valid[rand_int(0, static_cast<int>(valid.size()) - 1)];
}