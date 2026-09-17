#pragma once

#include "Reglages.h"

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <functional>

// Couleurs de asset/FondPrincipal.png, pour prolonger le décor au-delà de l'image
inline const sf::Color COULEUR_FOND(35, 35, 35);
inline const sf::Color COULEUR_PANNEAU(22, 22, 22);

// Fenêtre, ressources et réglages partagés par les menus et la partie
class Application {
public:
    sf::RenderWindow fenetre;
    Reglages reglages;
    sf::Font police;
    sf::Texture tuiles, fondJeu, logo, fondMenu;

    // Charge les ressources et les réglages puis ouvre la fenêtre. false si une ressource manque.
    bool Initialiser();

    void SauverReglages();

    // (Re)crée la fenêtre en plein écran ou en fenêtré selon reglages.pleinEcran
    void AppliquerPleinEcran();

    // Fermeture, redimensionnement et F11 (plein écran). Renvoie true si l'événement est traité.
    bool GererEvenement(const sf::Event& evenement);

    // Affiche l'image. Si la synchro verticale est ignorée par le pilote, limite la cadence
    // (~300 images/s, 30 sans le focus) pour ne pas occuper un cœur du processeur à 100 %.
    void Afficher();

    // Pixels à l'écran par unité logique
    float Echelle() const;

    // Zone visible en unités logiques : contient toujours 900x540, plus large ou plus haute selon la fenêtre
    sf::FloatRect ZoneVisible() const;

    // Dessine dans une texture de la taille de la fenêtre (fond des menus de pause et de fin)
    const sf::Texture& Capturer(const std::function<void(sf::RenderTarget&)>& dessiner);

private:
    sf::RenderTexture scene;
    std::filesystem::path cheminReglages;
    sf::Clock horlogeImage;

    void AjusterVue();
};
