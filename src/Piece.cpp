#include "Piece.h"

#include <cstddef>

namespace {

struct Definition {
    int taille;  // côté de la boîte dans laquelle la pièce tourne
    Cases cases; // cases en rotation 0
    int couleur; // n° de tuile dans asset/tiles.png
    int colonne; // colonne de la boîte à l'apparition
};

// Même ordre que TypePiece.
// Tuiles : 1 violet, 2 rouge, 3 vert, 4 jaune, 5 cyan, 6 orange, 7 bleu
constexpr std::array<Definition, NB_TYPES> DEFINITIONS = {{
    {4, {{{0, 1}, {1, 1}, {2, 1}, {3, 1}}}, 5, 3}, // I
    {2, {{{0, 0}, {1, 0}, {0, 1}, {1, 1}}}, 4, 4}, // O
    {3, {{{1, 0}, {0, 1}, {1, 1}, {2, 1}}}, 1, 3}, // T
    {3, {{{1, 0}, {2, 0}, {0, 1}, {1, 1}}}, 3, 3}, // S
    {3, {{{0, 0}, {1, 0}, {1, 1}, {2, 1}}}, 2, 3}, // Z
    {3, {{{0, 0}, {0, 1}, {1, 1}, {2, 1}}}, 7, 3}, // J
    {3, {{{2, 0}, {0, 1}, {1, 1}, {2, 1}}}, 6, 3}, // L
}};

using Tests = std::array<Case, 5>;

// Tables SRS officielles, notées avec y vers le HAUT comme dans la documentation.
// Indexées [rotation de départ][0 = horaire, 1 = anti-horaire].
constexpr Tests SRS_JLSTZ[4][2] = {
    {{{{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}}},  // 0 -> R
     {{{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}}}},    // 0 -> L
    {{{{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}}},      // R -> 2
     {{{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}}}},     // R -> 0
    {{{{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}}},     // 2 -> L
     {{{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}}}}, // 2 -> R
    {{{{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}},   // L -> 0
     {{{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}}},  // L -> 2
};

constexpr Tests SRS_I[4][2] = {
    {{{{0, 0}, {-2, 0}, {1, 0}, {-2, -1}, {1, 2}}},  // 0 -> R
     {{{0, 0}, {-1, 0}, {2, 0}, {-1, 2}, {2, -1}}}}, // 0 -> L
    {{{{0, 0}, {-1, 0}, {2, 0}, {-1, 2}, {2, -1}}},  // R -> 2
     {{{0, 0}, {2, 0}, {-1, 0}, {2, 1}, {-1, -2}}}}, // R -> 0
    {{{{0, 0}, {2, 0}, {-1, 0}, {2, 1}, {-1, -2}}},  // 2 -> L
     {{{0, 0}, {1, 0}, {-2, 0}, {1, -2}, {-2, 1}}}}, // 2 -> R
    {{{{0, 0}, {1, 0}, {-2, 0}, {1, -2}, {-2, 1}}},  // L -> 0
     {{{0, 0}, {-2, 0}, {1, 0}, {-2, -1}, {1, 2}}}}, // L -> 2
};

const Definition& Def(TypePiece type) {
    return DEFINITIONS[static_cast<std::size_t>(type)];
}

int Normaliser(int rotation) {
    return ((rotation % 4) + 4) % 4;
}

} // namespace

namespace piece {

Cases Forme(TypePiece type, int rotation) {
    const Definition& def = Def(type);
    Cases cases = def.cases;
    for (int r = 0; r < Normaliser(rotation); r++) {
        // Quart de tour horaire dans la boîte : (x, y) -> (taille-1-y, x)
        for (Case& c : cases) c = {def.taille - 1 - c.y, c.x};
    }
    return cases;
}

int Couleur(TypePiece type) {
    return Def(type).couleur;
}

int ColonneDepart(TypePiece type) {
    return Def(type).colonne;
}

std::array<Case, 5> Decalages(TypePiece type, int rotation, bool horaire) {
    if (type == TypePiece::O) return {}; // le carré ne se décale jamais

    const Tests& table = (type == TypePiece::I ? SRS_I : SRS_JLSTZ)[Normaliser(rotation)][horaire ? 0 : 1];
    Tests resultat = table;
    for (Case& c : resultat) c.y = -c.y; // passage en y vers le bas
    return resultat;
}

} // namespace piece
