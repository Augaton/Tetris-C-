#pragma once

#include "Piece.h"

#include <SFML/Graphics.hpp>
#include <array>
#include <random>

// Décor des menus : fond de nuit où des tétrominos tombent lentement, à plusieurs profondeurs,
// et logo du jeu écrit en blocs. Tout est tracé par le code, net à toutes les tailles.
class Decor {
public:
    Decor();

    // `zone` : partie visible de la vue ; `dt` : temps écoulé depuis l'image précédente
    void DessinerFond(sf::RenderTarget& cible, sf::FloatRect zone, float dt, bool daltonien);

    // TETRIS en blocs, une couleur de pièce par lettre, sur un panneau sombre
    void DessinerLogo(sf::RenderTarget& cible, sf::Vector2f centre, bool daltonien);

private:
    struct Chute {
        TypePiece type = TypePiece::I;
        float x = 0.f;          // 0 à 1 : position dans la largeur de la zone
        float y = 0.f;          // depuis le haut de la zone, en unités logiques
        float profondeur = 0.f; // 0 : au loin (petite, pâle, lente) ; 1 : tout près
        float angle = 0.f, rotation = 0.f; // degrés, degrés par seconde
    };

    // Rangées de la plus lointaine à la plus proche : les proches sont dessinées par-dessus
    std::array<Chute, 28> chutes{};
    bool place = false;
    std::mt19937 rng{std::random_device{}()};
    sf::VertexArray fond{sf::PrimitiveType::Triangles};
    sf::VertexArray logo{sf::PrimitiveType::Triangles};

    float Aleatoire(float min, float max);
    void Relancer(Chute& chute, float y);
};
