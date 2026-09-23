#include "Palette.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace {

// Même ordre que les tuiles : 1 violet (T), 2 rouge (Z), 3 vert (S), 4 jaune (O), 5 cyan (I), 6 orange (L), 7 bleu (J)
constexpr std::array<sf::Color, 8> NORMALE = {{
    {62, 130, 237}, {142, 104, 203}, {219, 45, 32}, {102, 191, 41},
    {244, 200, 36}, {48, 190, 229},  {235, 125, 36}, {62, 130, 237},
}};

// Okabe-Ito : pourpre, vermillon, vert bleuté, jaune, bleu ciel, orange, bleu
constexpr std::array<sf::Color, 8> DALTONIEN = {{
    {0, 114, 178},   {204, 121, 167}, {213, 94, 0},   {0, 158, 115},
    {240, 228, 66},  {86, 180, 233},  {230, 159, 0},  {0, 114, 178},
}};

float Luminance(sf::Color c) {
    return 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
}

} // namespace

namespace palette {

sf::Color Tuile(int tuile, bool daltonien) {
    const size_t i = static_cast<size_t>(std::clamp(tuile, 0, 7));
    return daltonien ? DALTONIEN[i] : NORMALE[i];
}

sf::Image Recolorer(const sf::Image& tuiles, unsigned tailleTuile) {
    sf::Image resultat = tuiles;
    const sf::Vector2u taille = tuiles.getSize();

    for (unsigned t = 1; t < 8 && (t + 1) * tailleTuile <= taille.x; t++) {
        const float base = std::max(1.f, Luminance(NORMALE[t]));
        const sf::Color cible = DALTONIEN[t];
        for (unsigned y = 0; y < std::min(tailleTuile, taille.y); y++) {
            for (unsigned x = t * tailleTuile; x < (t + 1) * tailleTuile; x++) {
                const sf::Color p = tuiles.getPixel({x, y});
                // Même rapport de luminosité que l'original : les reflets et les ombres sont conservés
                const float rapport = Luminance(p) / base;
                auto canal = [&](std::uint8_t c) { return static_cast<std::uint8_t>(std::clamp(c * rapport, 0.f, 255.f)); };
                resultat.setPixel({x, y}, {canal(cible.r), canal(cible.g), canal(cible.b), p.a});
            }
        }
    }
    return resultat;
}

} // namespace palette
