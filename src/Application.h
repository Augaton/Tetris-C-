#pragma once

#include "Reglages.h"
#include "Theme.h"

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <functional>

// Fenêtre, ressources et réglages partagés par les menus et la partie
class Application {
public:
    sf::RenderWindow fenetre;
    Reglages reglages;
    sf::Font police; // seule ressource chargée : tout le reste est dessiné par le code

    // Charge la police et les réglages puis ouvre la fenêtre. false si la police manque.
    bool Initialiser();

    void SauverReglages();

    // (Re)crée la fenêtre en plein écran ou en fenêtré selon reglages.pleinEcran
    void AppliquerPleinEcran();
    void AppliquerSynchroVerticale();

    // Fermeture, redimensionnement et F11 (plein écran). Renvoie true si l'événement est traité.
    bool GererEvenement(const sf::Event& evenement);

    // Affiche l'image puis attend l'échéance de la suivante : cadence régulière selon reglages.limiteImages
    // (300 images/s par défaut si la synchro verticale est coupée ou ignorée), 30 sans le focus.
    void Afficher();

    // Taille des textes des menus (option d'accessibilité)
    float FacteurTexte() const { return static_cast<float>(reglages.tailleTexte) / 100.f; }

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
    sf::Time prochaineImage;
    bool aLeFocus = true; // suivi par les événements : évite un appel au système à chaque image

    void AjusterVue();
};
