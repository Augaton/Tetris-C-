#pragma once

#include "Application.h"
#include "Classements.h"
#include "Jeu.h"
#include "Manette.h"

#include <SFML/Graphics.hpp>
#include <functional>
#include <optional>
#include <span>
#include <string>

// Ce que l'écran de fin de partie affiche
struct ResumePartie {
    ParametresPartie parametres;
    bool perdu = false;
    bool objectifAtteint = false;
    long long score = 0;
    int lignes = 0;
    int niveau = 0;
    float temps = 0.f;
    Statistiques stats;
    int rang = -1;                            // place dans le classement, -1 si hors classement
    std::optional<EntreeClassement> premier; // record du mode, après ajout de la partie
};

// Menus navigables à la souris, au clavier (flèches, Entrée, Échap) et à la manette
class Menu {
public:
    enum class Choix { Jouer, MenuPrincipal, Quitter };

    explicit Menu(Application& application);

    // Jouer (avec les paramètres choisis dans `parametres`) ou Quitter ; donne accès au reste
    Choix Principal(const Classements& classements, ParametresPartie& parametres);

    // Renvoie true si le joueur abandonne la partie
    bool Pause(const sf::Texture& scene);

    bool Confirmer(const sf::Texture& scene, const std::string& question);

    // 3, 2, 1 par-dessus la partie avant qu'elle reprenne
    void CompteARebours(const sf::Texture& scene);

    // Statistiques et classement ; Jouer = recommencer. `revoir` vide : pas de revisionnage possible.
    Choix FinDePartie(const sf::Texture& scene, const ResumePartie& resume, const std::function<void()>& revoir);

    // Prochain événement de la fenêtre ; les entrées de manette sont traduites en touches de menu
    std::optional<sf::Event> Lire();

private:
    // Fond d'un sous-menu : animé (défilement du menu principal) ou figé (capture floutée de la partie)
    struct Fond {
        std::function<void()> dessiner;
        bool anime;
    };

    // Ligne d'un écran de réglages : libellé (avec sa valeur) et modification (delta ±1, ou validation)
    struct ElementReglage {
        std::function<sf::String()> libelle;
        std::function<void(int delta, bool valide)> modifier;
    };

    Application& app;
    sf::Shader blurShader;
    bool shaderOk = false;
    sf::RenderTexture flouReduit, flouIntermediaire, fondFlou;
    sf::Text texte{app.police};
    sf::VertexArray formes{sf::PrimitiveType::Triangles}; // petites formes des titres et des cartes
    sf::Clock horlogeRepos; // depuis le dernier affichage d'un écran immobile
    manette::Traducteur traducteur;
    bool traduireManette = true; // désactivé pendant la saisie d'un bouton dans Commandes

    std::optional<ParametresPartie> ChoisirMode(const Fond& fond, const Classements& classements);
    void AfficherClassements(const Fond& fond, const Classements& classements);
    void Options(const Fond& fond);
    void EcranReglages(const Fond& fond, const sf::String& titre, std::span<const ElementReglage> elements);
    void Commandes(const Fond& fond);
    bool ConfirmerSurFond(const Fond& fond, const std::string& question);

    std::optional<sf::Event> ProchainEvenement(bool& aJour, bool anime);
    void AfficherImage(bool& aJour);

    void PreparerFlou(const sf::Texture& scene);
    void DessinerDansZone(const sf::Texture& texture);
    void DessinerFondFlou();
    void DessinerTitre(const sf::String& chaine, float y);
    void DessinerTexte(const sf::String& chaine, unsigned taille, float y, sf::Color couleur = sf::Color::White);
    void DessinerTexte(const sf::String& chaine, unsigned taille, sf::Vector2f centre, sf::Color couleur, float largeurMax);
};
