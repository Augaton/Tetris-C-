#pragma once

#include <SFML/Graphics.hpp>
#include <string>

// Convertit une chaîne UTF-8 (accents, flèches) pour SFML
sf::String Utf8(const std::string& texte);

// Rastérise le texte à sa taille réelle à l'écran (net quelle que soit la taille de la fenêtre),
// le ramène à sa taille logique puis le centre sur `centre`.
void PlacerTexte(sf::Text& texte, unsigned tailleLogique, sf::Vector2f centre, float echelle, float zoom = 1.f);

// Comme PlacerTexte, mais réduit le texte s'il dépasse `largeurMax` (unités logiques)
void PlacerTexteBorne(sf::Text& texte, unsigned tailleLogique, sf::Vector2f centre, float echelle, float largeurMax);
