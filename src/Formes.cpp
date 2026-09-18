#include "Formes.h"

#include <algorithm>
#include <cmath>

namespace formes {

void AjouterRectangleArrondi(sf::VertexArray& sommets, sf::FloatRect zone, float rayon, sf::Color couleur,
                             const sf::Transform& transformation) {
    if (zone.width <= 0.f || zone.height <= 0.f || couleur.a == 0) return;
    rayon = std::max(0.f, std::min({rayon, zone.width / 2.f, zone.height / 2.f}));

    // 4 arcs de SEGMENTS segments autour du centre
    const int SEGMENTS = 5;
    const float droite = zone.left + zone.width, bas = zone.top + zone.height;
    const sf::Vector2f centresArcs[4] = {
        {droite - rayon, zone.top + rayon}, {droite - rayon, bas - rayon},
        {zone.left + rayon, bas - rayon},   {zone.left + rayon, zone.top + rayon},
    };
    const sf::Vector2f centre = transformation.transformPoint(zone.left + zone.width / 2.f, zone.top + zone.height / 2.f);

    sf::Vector2f premier, precedent;
    for (int arc = 0; arc < 4; arc++) {
        for (int i = 0; i <= SEGMENTS; i++) {
            const float angle =
                (-90.f + 90.f * (static_cast<float>(arc) + static_cast<float>(i) / SEGMENTS)) * 3.14159265f / 180.f;
            const sf::Vector2f point =
                transformation.transformPoint(centresArcs[arc] + sf::Vector2f(std::cos(angle), std::sin(angle)) * rayon);
            if (arc == 0 && i == 0) {
                premier = point;
            } else {
                sommets.append(sf::Vertex(centre, couleur));
                sommets.append(sf::Vertex(precedent, couleur));
                sommets.append(sf::Vertex(point, couleur));
            }
            precedent = point;
        }
    }
    sommets.append(sf::Vertex(centre, couleur));
    sommets.append(sf::Vertex(precedent, couleur));
    sommets.append(sf::Vertex(premier, couleur));
}

} // namespace formes
