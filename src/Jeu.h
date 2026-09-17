#pragma once

#include "Constantes.h"
#include "Piece.h"
#include "Sac.h"

#include <array>
#include <optional>
#include <random>

// Règles du jeu, sans rendu ni SFML.
// La grille ne contient que les blocs posés : la pièce active est gardée à part.
class Jeu {
public:
    // 0 = case vide, sinon n° de tuile
    using Grille = std::array<std::array<int, cst::LARGEUR>, cst::HAUTEUR>;

    explicit Jeu(unsigned graine = std::random_device{}(), const Grille& depart = {});

    // Actions du joueur (renvoient true si la pièce a bougé)
    bool Deplacer(int dx);
    bool DescenteDouce();
    void ChuteRapide();
    bool Tourner(bool horaire);
    void Garder();

    // Fait avancer le temps : gravité, verrouillage et expiration du combo
    void MettreAJour(float dt);

    // Temps pendant lequel la pièce posée peut encore bouger avant d'être verrouillée
    void DefinirDelaiVerrouillage(float secondes) { delaiVerrouillage = secondes < 0.f ? 0.f : secondes; }

    const Grille& Plateau() const { return grille; }
    Cases CasesPiece() const;
    Cases CasesFantome() const;
    TypePiece PieceActive() const { return active.type; }
    TypePiece PieceSuivante() const { return suivante; }
    std::optional<TypePiece> PieceGardee() const { return garde; }
    bool GardeUtilisee() const { return gardeUtilisee; }

    long long Score() const { return score; }
    int Niveau() const { return niveau; }
    int Lignes() const { return lignes; }
    int Combo() const { return combo; }
    float ComboRestant() const { return comboRestant; }
    float IntervalleGravite() const;
    // 0 = pièce libre, 1 = sur le point d'être verrouillée
    float ProgressionVerrouillage() const;
    bool Perdu() const { return perdu; }

private:
    struct EtatPiece {
        TypePiece type;
        int rotation;
        int x, y; // position de la boîte de rotation
    };

    Grille grille;
    Sac sac;
    EtatPiece active{};
    TypePiece suivante{};
    std::optional<TypePiece> garde;
    bool gardeUtilisee = false;

    long long score = 0;
    int niveau = 0;
    int lignes = 0;
    int combo = 0;
    float comboRestant = 0.f;
    float chronoGravite = 0.f;
    bool perdu = false;

    // Verrouillage différé : chaque mouvement réussi au sol relance le délai, dans une limite
    // de REINITIALISATIONS_MAX (remise à zéro si la pièce atteint une ligne plus basse)
    static constexpr int REINITIALISATIONS_MAX = 15;
    float delaiVerrouillage = 0.5f;
    float chronoVerrouillage = 0.f;
    int reinitialisations = 0;
    int ligneLaPlusBasse = 0;

    static Cases Placer(const EtatPiece& piece);
    bool Libre(const Cases& cases) const;
    bool AuSol() const;
    void ApresMouvement();
    void Apparaitre(TypePiece type);
    void Verrouiller();
    int EffacerLignes();
    void AjouterLignes(int nombre);
};
