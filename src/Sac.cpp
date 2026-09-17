#include "Sac.h"

#include <algorithm>

Sac::Sac(unsigned graine) : rng(graine) {
    pieces.reserve(NB_TYPES);
}

TypePiece Sac::Tirer() {
    if (pieces.empty()) {
        for (int i = 0; i < NB_TYPES; i++) pieces.push_back(static_cast<TypePiece>(i));
        std::shuffle(pieces.begin(), pieces.end(), rng);
    }
    TypePiece type = pieces.back();
    pieces.pop_back();
    return type;
}
