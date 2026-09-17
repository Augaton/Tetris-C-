#pragma once

#include <array>

enum class TypePiece { I, O, T, S, Z, J, L };
inline constexpr int NB_TYPES = 7;

struct Case {
    int x, y;
};
using Cases = std::array<Case, 4>;

namespace piece {

// Cases occupées dans la boîte de rotation (rotation 0 à 3, sens horaire, y vers le bas)
Cases Forme(TypePiece type, int rotation);

// Numéro de la tuile dans asset/tiles.png
int Couleur(TypePiece type);

// Colonne de la boîte de rotation à l'apparition
int ColonneDepart(TypePiece type);

// Décalages SRS à essayer, dans l'ordre, pour tourner depuis `rotation` (y vers le bas)
std::array<Case, 5> Decalages(TypePiece type, int rotation, bool horaire);

} // namespace piece
