#pragma once

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
inline constexpr int TOUCHES_PAR_ACTION = 2;
inline constexpr int AUCUNE_TOUCHE = -1;

struct Bornes {
    int min, max, pas;
    int Limiter(int valeur) const { return valeur < min ? min : (valeur > max ? max : valeur); }
};

// Réglages du joueur. Les touches sont des codes entiers (sf::Keyboard::Key côté application)
// pour que ce fichier reste indépendant de SFML.
struct Reglages {
    using TableTouches = std::array<std::array<int, TOUCHES_PAR_ACTION>, NB_ACTIONS>;

    static constexpr Bornes BORNES_DAS{0, 500, 10};
    static constexpr Bornes BORNES_ARR{0, 200, 5};
    static constexpr Bornes BORNES_DESCENTE{0, 200, 5};
    static constexpr Bornes BORNES_VERROUILLAGE{0, 1000, 50};

    int dasMs = 170;
    int arrMs = 50;
    int descenteDouceMs = 50;
    int verrouillageMs = 500;
    bool fantome = true;
    bool effets = true;    // particules, éclats, textes flottants
    bool secousses = true; // tremblement du plateau (désactivable pour le confort visuel)
    bool pleinEcran = false;

    // [action][0] = touche principale, [1] = secondaire
    TableTouches touches = TouchesVides();

    const std::array<int, TOUCHES_PAR_ACTION>& Touches(Action action) const;

    // La touche devient la principale de l'action (l'ancienne principale passe en secondaire)
    // et est retirée des autres actions pour éviter les doublons.
    void AssignerTouche(Action action, int code);
    void EffacerTouches(Action action);
    std::optional<Action> ActionDe(int code) const;

    // Remet les valeurs numériques et les options par défaut, sans toucher aux touches
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
