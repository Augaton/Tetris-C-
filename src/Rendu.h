#pragma once

#include "Effets.h"
#include "Jeu.h"
#include "Reglages.h"
#include "Texte.h"

#include <SFML/Graphics.hpp>
#include <array>
#include <optional>

// Dessine une partie. Peut viser la fenêtre ou une RenderTexture (capture pour pause / fin).
class Rendu {
public:
    Rendu(const sf::Texture& tuiles, const sf::Texture& tuilesDaltonien, const sf::Texture& fond, const sf::Font& police);

    // Meilleur score au début de la partie : le score passe en doré quand il est battu
    void DefinirRecord(long long valeur) { record = valeur; }

    // `echelle` = pixels par unité logique, pour des textes nets
    void Dessiner(sf::RenderTarget& cible, const Jeu& jeu, float temps, float echelle, const Reglages& reglages,
                  Effets& effets);

private:
    // Texte dont la mise en page n'est refaite que si la chaîne ou l'échelle change
    struct TexteCache {
        explicit TexteCache(const sf::Font& police) : texte(police) {}
        sf::Text texte;
        sf::String chaine;
        float echelle = 0.f;
    };

    const sf::Texture& tuiles;
    const sf::Texture& tuilesDaltonien;
    const sf::Font& police;
    sf::Sprite fond;
    sf::RectangleShape limite;
    sf::RectangleShape masqueCommandes;
    // Tuiles regroupées : un appel de dessin pour le plateau, un pour les aperçus
    sf::VertexArray sommets{sf::PrimitiveType::Triangles};

    sf::VertexArray motifs{sf::PrimitiveType::Triangles}; // accessibilité : un motif par type de pièce
    bool motifsActifs = false;
    sf::VertexArray formes{sf::PrimitiveType::Triangles}; // badge du combo : formes arrondies en un seul appel de dessin

    float dernierTemps = 0.f;
    double scoreAffiche = 0.0;

    // Mouvements fluides : position affichée de la pièce (en cases), qui rattrape la position réelle
    sf::Vector2f positionAffichee;
    int numeroSuivi = -1;
    const Jeu* jeuSuivi = nullptr;

    float echellePrechargee = 0.f;

    TexteCache score{police}, lignes{police}, niveau{police};
    // Titres du fond redessinés (anglais, modes chronométrés)
    std::array<TexteCache, 4> etiquettes = Tableau<TexteCache, 4>(police);
    TexteCache piedMode{police}; // nom du mode sous la grille
    long long record = 0;

    // Badge du combo
    sf::Text texteCombo{police}, texteComboLabel{police, "COMBO"};
    int comboAffiche = 0;          // reste affiché pendant la disparition
    float tempsCombo = 10.f;       // depuis le dernier changement de valeur (animation d'apparition)
    float disparitionCombo = 0.f;  // avance quand le combo est perdu

    std::array<sf::Text, 5> texteCommandes = Tableau<sf::Text, 5>(police);
    Reglages::TableTouches touchesAffichees = Reglages::TouchesVides();
    float echelleCommandes = 0.f;
    Langue langueCommandes = Langue::Francais;

    // Rastérise à l'avance les glyphes utilisés en partie : pas d'à-coup au premier combo ou texte flottant
    void PrechargerGlyphes(float echelle);
    void AjouterRectangleArrondi(const sf::Transform& transformation, sf::FloatRect zone, float rayon, sf::Color couleur);
    void AjouterTuile(int couleur, sf::Vector2f position, sf::Color teinte = sf::Color::White, bool avecMotif = true);
    void AjouterApercu(std::optional<TypePiece> type, cst::Point centre, sf::Color teinte);
    void DessinerTexte(sf::RenderTarget& cible, TexteCache& cache, const sf::String& chaine, unsigned taille,
                       sf::Vector2f centre, float echelle, float largeurMax, sf::Color couleur = sf::Color::White);
    void DessinerInfos(sf::RenderTarget& cible, const Jeu& jeu, long long scoreAffichage, float echelle);
    void DessinerCommandes(sf::RenderTarget& cible, const Reglages& reglages, float echelle);
    void DessinerLimite(sf::RenderTarget& cible, const Jeu& jeu, float temps, const sf::RenderStates& etats);
    void DessinerCombo(sf::RenderTarget& cible, const Jeu& jeu, float temps, float dt, float echelle);
};
