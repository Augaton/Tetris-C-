#pragma once

#include <optional>
#include <string_view>

enum class Mode {
    Marathon, // niveaux croissants jusqu'à la défaite
    Sprint,   // SPRINT_LIGNES lignes le plus vite possible
    Ultra,    // meilleur score en ULTRA_DUREE_S secondes
    Zen,      // sans gravité ni défaite
};
inline constexpr int NB_MODES = 4;

namespace mode {

inline constexpr int SPRINT_LIGNES = 40;
inline constexpr float ULTRA_DUREE_S = 120.f;
inline constexpr int NIVEAU_DEPART_MAX = 19;

} // namespace mode

struct ParametresPartie {
    Mode mode = Mode::Marathon;
    int niveauDepart = 0;                     // Marathon uniquement
    int sprintLignes = mode::SPRINT_LIGNES;   // Sprint uniquement
};

namespace mode {

// Identifiant utilisé dans les fichiers ("marathon", "sprint"...)
std::string_view Identifiant(Mode m);
std::optional<Mode> DepuisIdentifiant(std::string_view identifiant);

} // namespace mode
