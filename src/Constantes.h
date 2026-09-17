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

// Taille d'une tuile dans asset/tiles.png
inline constexpr int TUILE = 18;

// Disposition, calée sur asset/FondPrincipal.png
inline constexpr Point PLATEAU = {360.f, 136.f};         // coin haut-gauche de la grille
inline constexpr Point APERCU_SUIVANT = {734.5f, 132.5f}; // centre du cadre "Suivant"
inline constexpr Point APERCU_GARDE = {158.5f, 195.f};    // centre du cadre "Gardé"
inline constexpr Point TEXTE_SCORE = {738.5f, 282.f};
inline constexpr Point TEXTE_LIGNES = {156.f, 345.f};
inline constexpr Point TEXTE_NIVEAU = {156.f, 470.f};
inline constexpr Point TEXTE_COMBO = {450.f, 150.f};
inline constexpr Point BARRE_COMBO = {450.f, 178.f};

// Progression
inline constexpr int LIGNES_PAR_NIVEAU = 10;
inline constexpr int NIVEAU_MAX = 29;

// Gravité : GRAVITE_INITIALE_S * GRAVITE_FACTEUR^niveau, bornée par GRAVITE_MIN_S
inline constexpr float GRAVITE_INITIALE_S = 0.8f;
inline constexpr float GRAVITE_FACTEUR = 0.85f;
inline constexpr float GRAVITE_MIN_S = 0.1f;

inline constexpr float COMBO_DUREE_S = 4.f;

// Écran de reprise après une pause
inline constexpr float COMPTE_A_REBOURS_S = 0.6f; // durée de chaque chiffre (3, 2, 1)

} // namespace cst
