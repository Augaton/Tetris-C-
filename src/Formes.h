#pragma once

#include <SFML/Graphics.hpp>

// Formes ajoutées à un tableau de sommets en triangles : tout un écran se dessine ainsi en quelques appels,
// net quelle que soit la taille de la fenêtre.
namespace formes {

// Rectangle aux coins arrondis (éventail de triangles)
void AjouterRectangleArrondi(sf::VertexArray& sommets, sf::FloatRect zone, float rayon, sf::Color couleur,
                             const sf::Transform& transformation = sf::Transform::Identity);

// Bordure d'un rectangle arrondi, d'épaisseur `epaisseur` vers l'intérieur de `zone`
void AjouterContourArrondi(sf::VertexArray& sommets, sf::FloatRect zone, float rayon, float epaisseur, sf::Color couleur,
                           const sf::Transform& transformation = sf::Transform::Identity);

// Quadrilatère (coins dans l'ordre du contour) sous forme de deux triangles : SFML 3 ne dessine plus de sf::Quads
void AjouterQuad(sf::VertexArray& sommets, const sf::Vertex& a, const sf::Vertex& b, const sf::Vertex& c,
                 const sf::Vertex& d);

// Bloc de pièce en relief : biseaux clairs en haut et à gauche, sombres à droite et en bas, face en dégradé.
// `teinte` module toutes les couleurs (fantôme translucide, pièce qui s'assombrit avant d'être posée...).
void AjouterMino(sf::VertexArray& sommets, sf::FloatRect zone, sf::Color couleur, sf::Color teinte = sf::Color::White,
                 const sf::Transform& transformation = sf::Transform::Identity);

} // namespace formes
