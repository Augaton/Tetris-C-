#pragma once

#include <SFML/Graphics.hpp>

namespace palette {

// Couleur d'une pièce (tuile n° 1 à 7), normale ou adaptée aux daltonismes courants (Okabe-Ito)
sf::Color Tuile(int tuile, bool daltonien);

// Couleur rapprochée de `cible` d'une fraction `t` (0 : inchangée, 1 : `cible`), opacité comprise
sf::Color Melanger(sf::Color couleur, sf::Color cible, float t);

} // namespace palette
