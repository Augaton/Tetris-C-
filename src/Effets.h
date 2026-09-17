#pragma once

#include "Jeu.h"

#include <SFML/Graphics.hpp>
#include <array>
#include <random>
#include <vector>

// Effets visuels légers : tout est calculé sur le processeur et dessiné en un seul appel
// (quads non texturés), sans shader. Nombre d'éléments borné, aucune allocation en jeu.
class Effets {
public:
    explicit Effets(const sf::Font& police);

    void Traiter(const std::vector<EvenementJeu>& evenements, bool effetsActifs, bool secoussesActives);
    void MettreAJour(float dt);

    // « NOUVEAU RECORD ! » quand le meilleur score est dépassé en cours de partie
    void AnnoncerRecord();

    // Décalage du plateau pour le tremblement (unités logiques)
    sf::Vector2f Secousse() const;

    // Éclats, traînées et particules, avec la transformation du plateau
    void DessinerPlateau(sf::RenderTarget& cible, const sf::RenderStates& etats);
    // Textes flottants (« TETRIS ! », « NIVEAU 5 »...)
    void DessinerTextes(sf::RenderTarget& cible, float echelle);

private:
    struct Particule {
        sf::Vector2f position, vitesse;
        sf::Color couleur;
        float vie, duree, taille;
    };
    struct Eclat { // rectangle blanc qui s'efface (ligne effacée, pièce verrouillée)
        sf::FloatRect zone;
        float vie, duree;
        bool seTasse; // la ligne se referme verticalement
    };
    struct Trainee { // sillage de la chute directe
        float x, haut, bas;
        sf::Color couleur;
        float vie, duree;
    };
    struct TexteFlottant {
        sf::Text texte;
        sf::Vector2f depart;
        unsigned taille = 0;
        float vie = 0.f, duree = 0.f;
    };

    static constexpr size_t MAX_PARTICULES = 320;
    static constexpr size_t MAX_ECLATS = 32;
    static constexpr size_t MAX_TRAINEES = 8;

    std::vector<Particule> particules;
    std::vector<Eclat> eclats;
    std::vector<Trainee> trainees;
    std::array<TexteFlottant, 4> textes;
    sf::VertexArray sommets{sf::Quads};

    float secousse = 0.f;
    float tempsSecousse = 0.f;
    std::mt19937 rng{std::random_device{}()};

    float Aleatoire(float min, float max);
    void AjouterParticule(sf::Vector2f position, sf::Vector2f vitesse, sf::Color couleur, float duree, float taille);
    void AjouterEclat(sf::FloatRect zone, float duree, bool seTasse);
    void AfficherTexte(const sf::String& chaine, sf::Vector2f position, unsigned taille, sf::Color couleur, float duree);
    void Secouer(float amplitude);
    void AjouterQuad(sf::FloatRect zone, sf::Color haut, sf::Color bas);
};
