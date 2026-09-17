#pragma once

#include "Piece.h"

#include <random>
#include <vector>

// Tirage « 7-bag » : les 7 pièces sortent dans un ordre mélangé avant de se répéter
class Sac {
public:
    explicit Sac(unsigned graine);
    TypePiece Tirer();

private:
    std::mt19937 rng;
    std::vector<TypePiece> pieces;
};
