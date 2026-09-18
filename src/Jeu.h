#pragma once

#include "Constantes.h"
#include "Mode.h"
#include "Piece.h"
#include "Sac.h"

#include <array>
#include <cstdint>
#include <optional>
#include <random>
#include <vector>

// Ce qui vient de se passer, pour les effets visuels. Taille fixe : pas d'allocation en jeu.
struct EvenementJeu {
    enum class Type { ChuteRapide, Verrouillage, Lignes, Niveau, Nettoyage }; // Nettoyage : Zen, pile vidée

    Type type;
    Cases cases{};   // ChuteRapide, Verrouillage : cases de la pièce posée
    int couleur = 0; // n° de tuile de la pièce
    int distance = 0; // ChuteRapide : lignes parcourues

    int nbLignes = 0;                                                   // Lignes
    std::array<int, 4> lignes{};                                        // indices avant effacement
    std::array<std::array<int, cst::LARGEUR>, 4> contenu{};             // tuiles des lignes effacées
    long long points = 0;                                               // points gagnés par ces lignes

    int niveau = 0; // Niveau : nouveau niveau
};

struct Statistiques {
    int pieces = 0;
    std::array<int, 4> lignesParType{}; // simples, doubles, triples, Tetris
    int comboMax = 0;
};

// Une entrée du journal de partie : chaque appel qui modifie le jeu, dans l'ordre.
// Rejoué sur un Jeu de même graine et mêmes paramètres, il reproduit la partie à l'identique.
struct Commande {
    enum class Type : std::uint8_t { Deplacer, DescenteDouce, ChuteRapide, Tourner, Garder, Temps, DelaiVerrouillage };
    Type type;
    std::int8_t argument = 0; // Deplacer : direction ; Tourner : 1 horaire, 0 anti-horaire
    float valeur = 0.f;       // Temps : dt ; DelaiVerrouillage : secondes
};

struct Enregistrement {
    unsigned graine = 0;
    ParametresPartie parametres;
    std::vector<Commande> commandes;
    bool complet = true; // faux si la partie a dépassé la taille maximale du journal
};

// Règles du jeu, sans rendu ni SFML.
// La grille ne contient que les blocs posés : la pièce active est gardée à part.
class Jeu {
public:
    // 0 = case vide, sinon n° de tuile
    using Grille = std::array<std::array<int, cst::LARGEUR>, cst::HAUTEUR>;

    explicit Jeu(unsigned graine = std::random_device{}(), const Grille& depart = {}, ParametresPartie parametres = {});
    Jeu(unsigned graine, ParametresPartie parametres) : Jeu(graine, {}, parametres) {}

    // Actions du joueur (renvoient true si la pièce a bougé)
    bool Deplacer(int dx);
    bool DescenteDouce();
    void ChuteRapide();
    bool Tourner(bool horaire);
    void Garder();

    // Fait avancer le temps : gravité, verrouillage et expiration du combo
    void MettreAJour(float dt);

    // Temps pendant lequel la pièce posée peut encore bouger avant d'être verrouillée
    void DefinirDelaiVerrouillage(float secondes);

    // Journal de la partie, pour la revoir ensuite (désactivé par défaut)
    void ActiverEnregistrement();
    const Enregistrement& Journal() const { return journal; }
    // Rejoue une commande d'un journal
    void Rejouer(const Commande& commande);

    // Événements depuis le dernier ViderEvenements() (les plus anciens sont oubliés au-delà de 32)
    const std::vector<EvenementJeu>& Evenements() const { return evenements; }
    void ViderEvenements() { evenements.clear(); }

    const Grille& Plateau() const { return grille; }
    Cases CasesPiece() const;
    Cases CasesFantome() const;
    TypePiece PieceActive() const { return active.type; }
    // Position de la boîte de rotation, et numéro qui change à chaque nouvelle pièce (animation du rendu)
    int PieceX() const { return active.x; }
    int PieceY() const { return active.y; }
    int NumeroPiece() const { return numeroPiece; }
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
    bool ObjectifAtteint() const { return objectifAtteint; } // Sprint terminé, Ultra écoulé
    bool Fini() const { return perdu || objectifAtteint; }

    const ParametresPartie& Parametres() const { return parametres; }
    float Temps() const { return temps; } // durée de jeu, pauses exclues
    float TempsRestant() const;           // Ultra uniquement
    const Statistiques& Stats() const { return stats; }

private:
    struct EtatPiece {
        TypePiece type;
        int rotation;
        int x, y; // position de la boîte de rotation
    };

    Grille grille;
    ParametresPartie parametres;
    unsigned graine;
    Sac sac;
    EtatPiece active{};
    TypePiece suivante{};
    std::optional<TypePiece> garde;
    bool gardeUtilisee = false;

    long long score = 0;
    int numeroPiece = 0;
    int niveau = 0;
    int lignes = 0;
    int combo = 0;
    float comboRestant = 0.f;
    float chronoGravite = 0.f;
    bool perdu = false;
    bool objectifAtteint = false;
    float temps = 0.f;
    Statistiques stats;

    // Journal borné (~16 Mo) : au-delà, la partie ne peut simplement pas être revue
    static constexpr size_t JOURNAL_MAX = 2'000'000;
    bool enregistrer = false;
    Enregistrement journal;
    void Noter(Commande::Type type, int argument = 0, float valeur = 0.f);

    // Verrouillage différé : chaque mouvement réussi au sol relance le délai, dans une limite
    // de REINITIALISATIONS_MAX (remise à zéro si la pièce atteint une ligne plus basse)
    static constexpr int REINITIALISATIONS_MAX = 15;
    float delaiVerrouillage = 0.5f;
    float chronoVerrouillage = 0.f;
    int reinitialisations = 0;
    int ligneLaPlusBasse = 0;

    std::vector<EvenementJeu> evenements;

    void Signaler(const EvenementJeu& evenement);
    void AjouterScore(long long points);
    static Cases Placer(const EtatPiece& piece);
    bool Libre(const Cases& cases) const;
    bool AuSol() const;
    void ApresMouvement();
    void Apparaitre(TypePiece type);
    void Verrouiller();
    int EffacerLignes();
    void AjouterLignes(int nombre);
};
