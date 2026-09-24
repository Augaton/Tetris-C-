#include "Decor.h"

#include "Formes.h"
#include "Palette.h"
#include "Theme.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <string_view>

namespace {

constexpr float COTE = 22.f;   // côté d'un bloc à la profondeur 1
constexpr float MARGE = 70.f;  // les pièces naissent et disparaissent hors de la zone visible

// Lettres de TETRIS sur 5 lignes, avec leur couleur de pièce (tuiles : 1 violet, 2 rouge, 3 vert, 4 jaune, 5 cyan, 6 orange)
struct Lettre {
    int couleur;
    std::array<std::string_view, 5> lignes;
};
constexpr std::array<Lettre, 6> LETTRES = {{
    {2, {"###", ".#.", ".#.", ".#.", ".#."}}, // T
    {6, {"###", "#..", "##.", "#..", "###"}}, // E
    {4, {"###", ".#.", ".#.", ".#.", ".#."}}, // T
    {3, {"##.", "#.#", "##.", "#.#", "#.#"}}, // R
    {5, {"#", "#", "#", "#", "#"}},           // I
    {1, {"###", "#..", "###", "..#", "###"}}, // S
}};

} // namespace

Decor::Decor() {
    fond.resize(30 * 4 * chutes.size() + 6);
    fond.clear();
    logo.resize(3000);
    logo.clear();
}

float Decor::Aleatoire(float min, float max) {
    return std::uniform_real_distribution<float>(min, max)(rng);
}

void Decor::Relancer(Chute& chute, float y) {
    chute.type = static_cast<TypePiece>(std::uniform_int_distribution<int>(0, NB_TYPES - 1)(rng));
    chute.x = Aleatoire(0.f, 1.f);
    chute.y = y;
    chute.angle = Aleatoire(0.f, 360.f);
    chute.rotation = Aleatoire(-14.f, 14.f);
}

void Decor::DessinerFond(sf::RenderTarget& cible, sf::FloatRect zone, float dt, bool daltonien) {
    // Première image : pièces réparties sur toute la hauteur, de la plus lointaine à la plus proche
    if (!place) {
        for (std::size_t i = 0; i < chutes.size(); i++) {
            chutes[i].profondeur = static_cast<float>(i) / static_cast<float>(chutes.size() - 1);
            Relancer(chutes[i], Aleatoire(-MARGE, zone.size.y + MARGE));
        }
        place = true;
    }

    fond.clear();
    const sf::Vector2f coin = zone.position, taille = zone.size;
    formes::AjouterQuad(fond, {coin, theme::FOND_HAUT}, {coin + sf::Vector2f(taille.x, 0.f), theme::FOND_HAUT},
                        {coin + taille, theme::FOND_BAS}, {coin + sf::Vector2f(0.f, taille.y), theme::FOND_BAS});

    for (Chute& chute : chutes) {
        // Les pièces proches sont plus grandes, plus nettes et tombent plus vite : effet de profondeur
        chute.y += (12.f + 38.f * chute.profondeur) * dt;
        chute.angle += chute.rotation * dt;
        if (chute.y > taille.y + MARGE) Relancer(chute, -MARGE);

        const float cote = COTE * (0.55f + 0.8f * chute.profondeur);
        const sf::Color teinte(255, 255, 255, static_cast<std::uint8_t>(18.f + 46.f * chute.profondeur));
        sf::Transform transformation;
        transformation.translate(coin + sf::Vector2f(chute.x * taille.x, chute.y)).rotate(sf::degrees(chute.angle));

        // Pièce centrée sur son point de rotation
        const Cases cases = piece::Forme(chute.type, 0);
        const auto [minX, maxX] = std::ranges::minmax(cases | std::views::transform(&Case::x));
        const auto [minY, maxY] = std::ranges::minmax(cases | std::views::transform(&Case::y));
        const sf::Vector2f milieu(static_cast<float>(minX + maxX + 1) / 2.f, static_cast<float>(minY + maxY + 1) / 2.f);
        const sf::Color couleur = palette::Tuile(piece::Couleur(chute.type), daltonien);
        for (const Case& c : cases) {
            const sf::Vector2f position = (sf::Vector2f(static_cast<float>(c.x), static_cast<float>(c.y)) - milieu) * cote;
            formes::AjouterMino(fond, {position, {cote, cote}}, couleur, teinte, transformation);
        }
    }
    cible.draw(fond);
}

void Decor::DessinerLogo(sf::RenderTarget& cible, sf::Vector2f centre, bool daltonien) {
    constexpr float cote = 20.f, marge = 20.f;
    std::size_t colonnes = LETTRES.size() - 1; // une colonne vide entre deux lettres
    for (const Lettre& lettre : LETTRES) colonnes += lettre.lignes[0].size();
    const sf::Vector2f taille(static_cast<float>(colonnes) * cote, 5.f * cote);
    const sf::Vector2f coin = centre - taille / 2.f;

    logo.clear();
    const sf::FloatRect panneau(coin - sf::Vector2f(marge, marge), taille + sf::Vector2f(2.f * marge, 2.f * marge));
    formes::AjouterRectangleArrondi(logo, panneau, 18.f, theme::PANNEAU);
    formes::AjouterContourArrondi(logo, panneau, 18.f, 1.f, theme::BORD);

    // Ombre portée sous les blocs, puis les blocs
    for (const bool ombre : {true, false}) {
        float x = coin.x;
        for (const Lettre& lettre : LETTRES) {
            for (std::size_t ligne = 0; ligne < lettre.lignes.size(); ligne++) {
                for (std::size_t colonne = 0; colonne < lettre.lignes[ligne].size(); colonne++) {
                    if (lettre.lignes[ligne][colonne] != '#') continue;
                    const sf::Vector2f position(x + static_cast<float>(colonne) * cote, coin.y + static_cast<float>(ligne) * cote);
                    if (ombre)
                        formes::AjouterRectangleArrondi(logo, {position + sf::Vector2f(3.f, 3.f), {cote, cote}}, 2.f,
                                                        {0, 0, 0, 120});
                    else
                        formes::AjouterMino(logo, {position, {cote, cote}}, palette::Tuile(lettre.couleur, daltonien));
                }
            }
            x += static_cast<float>(lettre.lignes[0].size() + 1) * cote;
        }
    }
    cible.draw(logo);
}
