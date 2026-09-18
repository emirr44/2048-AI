#pragma once
#include "../core/game2048.h"
#include "../utils/helpers.h"

class RandomAI {
public:
    Direction get_move(const Game2048& game);
};