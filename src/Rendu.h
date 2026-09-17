#pragma once

#include "Effets.h"
#include "Jeu.h"
#include "Reglages.h"

#include <SFML/Graphics.hpp>
#include <array>
#include <optional>

// Dessine une partie. Peut viser la fenêtre ou une RenderTexture (capture pour pause / fin).
class Rendu {
public:
    Rendu(const sf::Texture& tuiles, const sf::Texture& fond, const sf::Font& police);

    // `echelle` = pixels par unité logique, pour des textes nets
    void Dessiner(sf::RenderTarget& cible, const Jeu& jeu, float temps, float echelle, const Reglages& reglages,
                  Effets& effets);

private:
    // Texte dont la chaîne n'est reconstruite que si la valeur ou l'échelle change
    struct Nombre {
        sf::Text texte;
        long long valeur = -1;
        float echelle = 0.f;
    };

    const sf::Texture& tuiles;
    sf::Sprite fond;
    sf::RectangleShape limite;
    sf::RectangleShape masqueCommandes;
    sf::VertexArray sommets{sf::Quads}; // tuiles regroupées : un appel de dessin pour le plateau, un pour les aperçus

    float dernierTemps = 0.f;
    double scoreAffiche = 0.0;

    Nombre score, lignes, niveau;
    sf::Text texteCombo;
    int comboAffiche = 0;

    std::array<sf::Text, 5> texteCommandes;
    Reglages::TableTouches touchesAffichees = Reglages::TouchesVides();
    float echelleCommandes = 0.f;

    void AjouterTuile(int couleur, sf::Vector2f position, sf::Color teinte = sf::Color::White);
    void AjouterApercu(std::optional<TypePiece> type, cst::Point centre, sf::Color teinte);
    void DessinerNombre(sf::RenderTarget& cible, Nombre& nombre, long long valeur, cst::Point centre, float echelle);
    void DessinerCommandes(sf::RenderTarget& cible, const Reglages& reglages, float echelle);
    void DessinerCombo(sf::RenderTarget& cible, const Jeu& jeu, float temps, float echelle);
};
