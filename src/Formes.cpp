#include "Formes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace formes {

void AjouterRectangleArrondi(sf::VertexArray& sommets, sf::FloatRect zone, float rayon, sf::Color couleur,
                             const sf::Transform& transformation) {
    if (zone.size.x <= 0.f || zone.size.y <= 0.f || couleur.a == 0) return;
    rayon = std::max(0.f, std::min({rayon, zone.size.x / 2.f, zone.size.y / 2.f}));

    // 4 arcs de SEGMENTS segments autour du centre
    constexpr int SEGMENTS = 5;
    const sf::Vector2f hautGauche = zone.position, basDroite = zone.position + zone.size;
    const std::array<sf::Vector2f, 4> centresArcs = {{
        {basDroite.x - rayon, hautGauche.y + rayon}, {basDroite.x - rayon, basDroite.y - rayon},
        {hautGauche.x + rayon, basDroite.y - rayon}, {hautGauche.x + rayon, hautGauche.y + rayon},
    }};
    const sf::Vector2f centre = transformation.transformPoint(zone.getCenter());

    sf::Vector2f premier, precedent;
    for (int arc = 0; arc < 4; arc++) {
        for (int i = 0; i <= SEGMENTS; i++) {
            const float angle = (-90.f + 90.f * (static_cast<float>(arc) + static_cast<float>(i) / SEGMENTS)) *
                                std::numbers::pi_v<float> / 180.f;
            const sf::Vector2f point = transformation.transformPoint(
                centresArcs[static_cast<size_t>(arc)] + sf::Vector2f(std::cos(angle), std::sin(angle)) * rayon);
            if (arc == 0 && i == 0) {
                premier = point;
            } else {
                sommets.append({centre, couleur});
                sommets.append({precedent, couleur});
                sommets.append({point, couleur});
            }
            precedent = point;
        }
    }
    sommets.append({centre, couleur});
    sommets.append({precedent, couleur});
    sommets.append({premier, couleur});
}

void AjouterQuad(sf::VertexArray& sommets, const sf::Vertex& a, const sf::Vertex& b, const sf::Vertex& c,
                 const sf::Vertex& d) {
    for (const sf::Vertex& sommet : {a, b, c, a, c, d}) sommets.append(sommet);
}

} // namespace formes
