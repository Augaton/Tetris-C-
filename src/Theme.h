#pragma once

#include <SFML/Graphics/Color.hpp>

// Couleurs de l'interface, partagées par la partie et les menus
namespace theme {

// Arrière-plan : dégradé vertical, du violet nuit au presque noir
inline constexpr sf::Color FOND_HAUT{30, 27, 48};
inline constexpr sf::Color FOND_BAS{11, 11, 18};

// Cartes et encadrés
inline constexpr sf::Color PANNEAU{17, 17, 29, 220};
inline constexpr sf::Color BORD{255, 255, 255, 28};

// Plateau : fond, quadrillage et cadre
inline constexpr sf::Color PLATEAU{7, 7, 13, 245};
inline constexpr sf::Color GRILLE{255, 255, 255, 13};
inline constexpr sf::Color CADRE{150, 150, 200, 110};

// Textes
inline constexpr sf::Color TEXTE{255, 255, 255};
inline constexpr sf::Color ETIQUETTE{140, 140, 170}; // petits titres en capitales espacées
inline constexpr sf::Color DISCRET{170, 170, 185};   // aides, informations secondaires
inline constexpr sf::Color OR{255, 204, 0};
inline constexpr sf::Color ALERTE{255, 90, 70};

} // namespace theme
