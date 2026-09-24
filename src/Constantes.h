#pragma once

// Constantes du jeu. Aucune dépendance à SFML pour que la logique reste testable.
namespace cst {

struct Point {
    float x, y;
};

// Plateau
inline constexpr int LARGEUR = 10;
inline constexpr int HAUTEUR = 20;
inline constexpr int LIGNES_ZONE_LIMITE = 4; // lignes au-dessus de la ligne rouge

// Zone de jeu en coordonnées logiques : la vue s'élargit pour remplir la fenêtre
inline constexpr unsigned FENETRE_LARGEUR = 900;
inline constexpr unsigned FENETRE_HAUTEUR = 540;

// Côté d'une case du plateau à l'écran (unités logiques)
inline constexpr int TUILE = 22;

// Coin haut-gauche de la grille : le plateau est centré dans la zone logique
inline constexpr Point PLATEAU = {(FENETRE_LARGEUR - TUILE * LARGEUR) / 2.f, (FENETRE_HAUTEUR - TUILE * HAUTEUR) / 2.f};

// Progression
inline constexpr int LIGNES_PAR_NIVEAU = 10;
inline constexpr int NIVEAU_MAX = 29;

// Gravité : GRAVITE_INITIALE_S * GRAVITE_FACTEUR^niveau, bornée par GRAVITE_MIN_S
inline constexpr float GRAVITE_INITIALE_S = 0.8f;
inline constexpr float GRAVITE_FACTEUR = 0.85f;
inline constexpr float GRAVITE_MIN_S = 0.1f;

inline constexpr float COMBO_DUREE_S = 4.f;
inline constexpr float COMBO_DISPARITION_S = 0.25f; // fondu du badge quand le combo est perdu

// Écran de reprise après une pause
inline constexpr float COMPTE_A_REBOURS_S = 0.6f; // durée de chaque chiffre (3, 2, 1)

} // namespace cst
