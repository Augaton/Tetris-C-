#pragma once

#include "Mode.h"

#include <array>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>

enum class Action {
    Gauche,
    Droite,
    DescenteDouce,
    ChuteRapide,
    TournerHoraire,
    TournerAntiHoraire,
    Garder,
    Pause,
    Abandonner,
};
inline constexpr int NB_ACTIONS = 9;

enum class Langue { Francais, Anglais };
inline constexpr int TOUCHES_PAR_ACTION = 2;
inline constexpr int AUCUNE_TOUCHE = -1;

struct Bornes {
    int min, max, pas;
    int Limiter(int valeur) const { return valeur < min ? min : (valeur > max ? max : valeur); }
};

// Réglages du joueur. Touches et boutons sont des codes entiers (clavier et manette côté application,
// dans des plages distinctes) pour que ce fichier reste indépendant de SFML.
struct Reglages {
    using TableTouches = std::array<std::array<int, TOUCHES_PAR_ACTION>, NB_ACTIONS>;

    static constexpr Bornes BORNES_DAS{0, 500, 10};
    static constexpr Bornes BORNES_ARR{0, 200, 5};
    static constexpr Bornes BORNES_DESCENTE{0, 200, 5};
    static constexpr Bornes BORNES_VERROUILLAGE{0, 1000, 50};
    static constexpr Bornes BORNES_LIMITE_IMAGES{0, 500, 1}; // 0 = automatique
    static constexpr Bornes BORNES_TAILLE_TEXTE{100, 130, 15}; // % de la taille des textes des menus

    int dasMs = 170;
    int arrMs = 50;
    int descenteDouceMs = 50;
    int verrouillageMs = 500;
    bool fantome = true;
    bool mouvementsFluides = true; // la pièce glisse d'une case à l'autre au lieu de sauter
    bool synchroVerticale = true;  // désactivée : latence plus faible, déchirures possibles
    int limiteImages = 0;          // images/s max ; 0 = automatique (synchro, sinon garde-fou à 300)
    bool effets = true;    // particules, éclats, textes flottants
    bool secousses = true; // tremblement du plateau (désactivable pour le confort visuel)
    bool pleinEcran = false;
    bool rotationAnticipee = false; // IRS/IHS : rotation et garde tenues appliquées dès l'apparition

    // Accessibilité
    bool daltonien = false; // palette Okabe-Ito, distinguable avec les daltonismes courants
    bool motifs = false;    // un motif par type de pièce, en plus de la couleur
    int tailleTexte = 100;  // % pour les menus
    Langue langue = Langue::Francais;

    // Dernière partie choisie
    Mode mode = Mode::Marathon;
    int niveauDepart = 0;

    // [action][0] = principale, [1] = secondaire ; clavier et manette séparés
    TableTouches touches = TouchesVides();
    TableTouches manette = TouchesVides();

    const std::array<int, TOUCHES_PAR_ACTION>& Touches(Action action) const;
    const std::array<int, TOUCHES_PAR_ACTION>& Boutons(Action action) const;

    // La touche (ou le bouton) devient la principale de l'action (l'ancienne passe en secondaire)
    // et est retirée des autres actions pour éviter les doublons.
    void AssignerTouche(Action action, int code);
    void AssignerBouton(Action action, int code);
    void EffacerTouches(Action action);
    void EffacerBoutons(Action action);
    // Cherche dans les touches puis dans les boutons de manette
    std::optional<Action> ActionDe(int code) const;

    // Remet les valeurs numériques et les options par défaut, sans toucher aux touches ni à la langue
    void ReinitialiserOptions();

    static TableTouches TouchesVides();
};

namespace reglages {

using NomVersCode = std::function<std::optional<int>(const std::string&)>;
using CodeVersNom = std::function<std::string(int)>;

// Identifiant utilisé dans le fichier ("gauche", "chute_rapide"...)
const char* Identifiant(Action action);

std::filesystem::path CheminFichier();

// Format « cle = valeur » ; lignes inconnues ou invalides ignorées, valeurs bornées.
Reglages Analyser(const std::string& contenu, const Reglages& defauts, const NomVersCode& codeTouche);
std::string Serialiser(const Reglages& reglages, const CodeVersNom& nomTouche);

Reglages Charger(const std::filesystem::path& chemin, const Reglages& defauts, const NomVersCode& codeTouche);
bool Sauvegarder(const std::filesystem::path& chemin, const Reglages& reglages, const CodeVersNom& nomTouche);

} // namespace reglages
