#include "Palette.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace {

// Numéros des tuiles : 1 violet (T), 2 rouge (Z), 3 vert (S), 4 jaune (O), 5 cyan (I), 6 orange (L), 7 bleu (J)
constexpr std::array<sf::Color, 8> NORMALE = {{
    {62, 130, 237}, {142, 104, 203}, {219, 45, 32}, {102, 191, 41},
    {244, 200, 36}, {48, 190, 229},  {235, 125, 36}, {62, 130, 237},
}};

// Okabe-Ito : pourpre, vermillon, vert bleuté, jaune, bleu ciel, orange, bleu
constexpr std::array<sf::Color, 8> DALTONIEN = {{
    {0, 114, 178},   {204, 121, 167}, {213, 94, 0},   {0, 158, 115},
    {240, 228, 66},  {86, 180, 233},  {230, 159, 0},  {0, 114, 178},
}};

} // namespace

namespace palette {

sf::Color Tuile(int tuile, bool daltonien) {
    const size_t i = static_cast<size_t>(std::clamp(tuile, 0, 7));
    return daltonien ? DALTONIEN[i] : NORMALE[i];
}

sf::Color Melanger(sf::Color couleur, sf::Color cible, float t) {
    const auto canal = [t](std::uint8_t de, std::uint8_t vers) {
        return static_cast<std::uint8_t>(std::lround(std::lerp(static_cast<float>(de), static_cast<float>(vers), t)));
    };
    return {canal(couleur.r, cible.r), canal(couleur.g, cible.g), canal(couleur.b, cible.b), canal(couleur.a, cible.a)};
}

} // namespace palette
