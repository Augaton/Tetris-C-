#pragma once

#include <SFML/Graphics.hpp>

namespace formes {

// Ajoute un rectangle aux coins arrondis (éventail de triangles) à un tableau de sommets en sf::Triangles.
// Tout un écran de formes se dessine ainsi en un seul appel.
void AjouterRectangleArrondi(sf::VertexArray& sommets, sf::FloatRect zone, float rayon, sf::Color couleur,
                             const sf::Transform& transformation = sf::Transform::Identity);

} // namespace formes
