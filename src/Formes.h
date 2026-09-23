#pragma once

#include <SFML/Graphics.hpp>

namespace formes {

// Ajoute un rectangle aux coins arrondis (éventail de triangles) à un tableau de sommets en triangles.
// Tout un écran de formes se dessine ainsi en un seul appel.
void AjouterRectangleArrondi(sf::VertexArray& sommets, sf::FloatRect zone, float rayon, sf::Color couleur,
                             const sf::Transform& transformation = sf::Transform::Identity);

// Ajoute un quadrilatère (coins dans l'ordre du contour) sous forme de deux triangles :
// SFML 3 ne dessine plus de sf::Quads
void AjouterQuad(sf::VertexArray& sommets, const sf::Vertex& a, const sf::Vertex& b, const sf::Vertex& c,
                 const sf::Vertex& d);

} // namespace formes
