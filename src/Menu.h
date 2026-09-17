#pragma once

#include "Application.h"

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>

// Menus navigables à la souris et au clavier (flèches, Entrée, Échap)
class Menu {
public:
    enum class Choix { Jouer, MenuPrincipal, Quitter };

    explicit Menu(Application& app);

    // Jouer ou Quitter ; donne accès aux options et aux commandes
    Choix Principal(long long meilleurScore);

    // Renvoie true si le joueur abandonne la partie
    bool Pause(const sf::Texture& scene);

    bool Confirmer(const sf::Texture& scene, const std::string& question);

    // 3, 2, 1 par-dessus la partie avant qu'elle reprenne
    void CompteARebours(const sf::Texture& scene);

    // Jouer = recommencer
    Choix Perdu(const sf::Texture& scene, long long score, long long meilleurScore, bool nouveauRecord);

private:
    using DessinFond = std::function<void()>;

    Application& app;
    sf::Shader blurShader;
    bool shaderOk = false;
    sf::RenderTexture flouReduit, flouIntermediaire, fondFlou;
    sf::Text texte;

    void Options(const DessinFond& fond);
    void Commandes(const DessinFond& fond);
    bool ConfirmerSurFond(const DessinFond& fond, const std::string& question);

    void PreparerFlou(const sf::Texture& scene);
    void DessinerDansZone(const sf::Texture& texture);
    void DessinerTexte(const sf::String& chaine, unsigned taille, float y, sf::Color couleur = sf::Color::White);
};
