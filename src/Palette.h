#pragma once

#include <SFML/Graphics.hpp>

namespace palette {

// Couleur dominante d'une tuile (n° 1 à 7), normale ou adaptée aux daltonismes courants (Okabe-Ito)
sf::Color Tuile(int tuile, bool daltonien);

// Recolore asset/tiles.png avec la palette Okabe-Ito en gardant le relief (reflets et ombres)
sf::Image Recolorer(const sf::Image& tuiles, unsigned tailleTuile);

} // namespace palette
