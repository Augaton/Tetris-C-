#pragma once

#include "Effets.h"
#include "Jeu.h"
#include "Reglages.h"
#include "Texte.h"

#include <SFML/Graphics.hpp>
#include <array>
#include <optional>

// Dessine une partie. Peut viser la fenêtre ou une RenderTexture (capture pour pause / fin).
// Tout est tracé par le code, net quelle que soit la taille de la fenêtre.
class Rendu {
public:
    explicit Rendu(const sf::Font& policeTexte);

    // Meilleur score au début de la partie : le score passe en doré quand il est battu
    void DefinirRecord(long long valeur) { record = valeur; }

    // `echelle` = pixels par unité logique, pour des textes nets
    void Dessiner(sf::RenderTarget& cible, const Jeu& jeu, float temps, float echelle, const Reglages& reglages,
                  Effets& effets);

private:
    enum class Alignement { Gauche, Centre, Droite };

    // Texte dont la mise en page n'est refaite que si la chaîne ou l'échelle change
    struct TexteCache {
        explicit TexteCache(const sf::Font& police) : texte(police) {}
        sf::Text texte;
        sf::String chaine;
        float echelle = 0.f;
    };

    const sf::Font& police;
    sf::VertexArray decor{sf::PrimitiveType::Triangles};   // arrière-plan et encadrés, fixes
    sf::VertexArray plateau{sf::PrimitiveType::Triangles}; // cadre, quadrillage et blocs : tremblent ensemble
    sf::VertexArray motifs{sf::PrimitiveType::Triangles};  // accessibilité : un motif par type de pièce
    bool motifsActifs = false;
    sf::VertexArray formes{sf::PrimitiveType::Triangles};  // pièces gardée et suivantes, badge du combo

    float dernierTemps = 0.f;
    double scoreAffiche = 0.0;

    // Mouvements fluides : position affichée de la pièce (en cases), qui rattrape la position réelle
    sf::Vector2f positionAffichee;
    int numeroSuivi = -1;
    const Jeu* jeuSuivi = nullptr;

    float echellePrechargee = 0.f;

    // Petits titres des encadrés (RÉSERVE, SUIVANTES, SCORE...), valeurs, nom du mode
    std::array<TexteCache, 6> etiquettes = Tableau<TexteCache, 6>(police);
    std::array<TexteCache, 3> valeurs = Tableau<TexteCache, 3>(police);
    TexteCache piedMode{police};
    long long record = 0;

    // Badge du combo
    sf::Text texteCombo{police}, texteComboLabel{police, "COMBO"};
    int comboAffiche = 0;          // reste affiché pendant la disparition
    float tempsCombo = 10.f;       // depuis le dernier changement de valeur (animation d'apparition)
    float disparitionCombo = 0.f;  // avance quand le combo est perdu

    // Commandes : action à gauche, touche à droite
    std::array<TexteCache, 6> actionsCommandes = Tableau<TexteCache, 6>(police);
    std::array<TexteCache, 6> touchesCommandes = Tableau<TexteCache, 6>(police);

    // Rastérise à l'avance les glyphes utilisés en partie : pas d'à-coup au premier combo ou texte flottant
    void PrechargerGlyphes(float echelle);
    void AjouterBloc(sf::VertexArray& sommets, int couleur, sf::FloatRect zone, sf::Color teinte, bool daltonien,
                     bool avecMotif = true);
    void AjouterApercu(std::optional<TypePiece> type, sf::Vector2f centre, float cote, sf::Color teinte, bool daltonien);
    void DessinerTexte(sf::RenderTarget& cible, TexteCache& cache, const sf::String& chaine, unsigned taille,
                       sf::Vector2f point, float echelle, float largeurMax, sf::Color couleur = sf::Color::White,
                       Alignement alignement = Alignement::Centre);
    void DessinerDecor(sf::RenderTarget& cible);
    void DessinerPlateau(sf::RenderTarget& cible, const Jeu& jeu, float temps, float dt, const Reglages& reglages,
                         Effets& effets);
    void DessinerApercus(sf::RenderTarget& cible, const Jeu& jeu, const Reglages& reglages);
    void DessinerInfos(sf::RenderTarget& cible, const Jeu& jeu, long long scoreAffichage, float echelle);
    void DessinerCommandes(sf::RenderTarget& cible, const Reglages& reglages, float echelle);
    void DessinerCombo(sf::RenderTarget& cible, const Jeu& jeu, float temps, float dt, float echelle);
};
