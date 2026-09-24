#include "Formes.h"

#include "Palette.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>

namespace {

constexpr int SEGMENTS = 5; // segments par quart de cercle
using Contour = std::array<sf::Vector2f, 4 * (SEGMENTS + 1)>;

float Rayon(sf::FloatRect zone, float rayon) {
    return std::max(0.f, std::min({rayon, zone.size.x / 2.f, zone.size.y / 2.f}));
}

// Bord d'un rectangle arrondi, dans le sens horaire à partir du haut du coin haut-droit
Contour PointsContour(sf::FloatRect zone, float rayon) {
    const sf::Vector2f hautGauche = zone.position, basDroite = zone.position + zone.size;
    const std::array<sf::Vector2f, 4> centresArcs = {{
        {basDroite.x - rayon, hautGauche.y + rayon}, {basDroite.x - rayon, basDroite.y - rayon},
        {hautGauche.x + rayon, basDroite.y - rayon}, {hautGauche.x + rayon, hautGauche.y + rayon},
    }};
    Contour points;
    for (std::size_t arc = 0; arc < centresArcs.size(); arc++) {
        for (int i = 0; i <= SEGMENTS; i++) {
            const float angle = (-90.f + 90.f * (static_cast<float>(arc) + static_cast<float>(i) / SEGMENTS)) *
                                std::numbers::pi_v<float> / 180.f;
            points[arc * (SEGMENTS + 1) + static_cast<std::size_t>(i)] =
                centresArcs[arc] + sf::Vector2f(std::cos(angle), std::sin(angle)) * rayon;
        }
    }
    return points;
}

sf::Color Eclaircir(sf::Color couleur, float t) {
    return palette::Melanger(couleur, sf::Color::White, t);
}

sf::Color Assombrir(sf::Color couleur, float t) {
    return palette::Melanger(couleur, sf::Color::Black, t);
}

} // namespace

namespace formes {

void AjouterRectangleArrondi(sf::VertexArray& sommets, sf::FloatRect zone, float rayon, sf::Color couleur,
                             const sf::Transform& transformation) {
    if (zone.size.x <= 0.f || zone.size.y <= 0.f || couleur.a == 0) return;
    const Contour points = PointsContour(zone, Rayon(zone, rayon));
    const sf::Vector2f centre = transformation.transformPoint(zone.getCenter());
    for (std::size_t i = 0; i < points.size(); i++) {
        sommets.append({centre, couleur});
        sommets.append({transformation.transformPoint(points[i]), couleur});
        sommets.append({transformation.transformPoint(points[(i + 1) % points.size()]), couleur});
    }
}

void AjouterContourArrondi(sf::VertexArray& sommets, sf::FloatRect zone, float rayon, float epaisseur, sf::Color couleur,
                           const sf::Transform& transformation) {
    if (zone.size.x <= 2.f * epaisseur || zone.size.y <= 2.f * epaisseur || couleur.a == 0) return;
    const float rayonExterieur = Rayon(zone, rayon);
    const sf::FloatRect interieur(zone.position + sf::Vector2f(epaisseur, epaisseur),
                                  zone.size - sf::Vector2f(2.f * epaisseur, 2.f * epaisseur));
    const Contour dehors = PointsContour(zone, rayonExterieur);
    const Contour dedans = PointsContour(interieur, std::max(0.f, rayonExterieur - epaisseur));
    for (std::size_t i = 0; i < dehors.size(); i++) {
        const std::size_t j = (i + 1) % dehors.size();
        AjouterQuad(sommets, {transformation.transformPoint(dehors[i]), couleur},
                    {transformation.transformPoint(dehors[j]), couleur}, {transformation.transformPoint(dedans[j]), couleur},
                    {transformation.transformPoint(dedans[i]), couleur});
    }
}

void AjouterQuad(sf::VertexArray& sommets, const sf::Vertex& a, const sf::Vertex& b, const sf::Vertex& c,
                 const sf::Vertex& d) {
    for (const sf::Vertex& sommet : {a, b, c, a, c, d}) sommets.append(sommet);
}

void AjouterMino(sf::VertexArray& sommets, sf::FloatRect zone, sf::Color couleur, sf::Color teinte,
                 const sf::Transform& transformation) {
    // Un joint d'1/18 de case à droite et en bas sépare les blocs voisins
    const float joint = zone.size.x / 18.f;
    const sf::Vector2f a = zone.position, d = zone.position + zone.size - sf::Vector2f(joint, joint);
    const float biseau = (d.x - a.x) * 0.235f;
    const sf::Vector2f b = a + sf::Vector2f(biseau, biseau), c = d - sf::Vector2f(biseau, biseau);

    const auto sommet = [&](float x, float y, sf::Color nuance) {
        return sf::Vertex{transformation.transformPoint({x, y}), nuance * teinte};
    };
    // Nuances de l'ancienne tuile dessinée, rapportées à la couleur de la pièce
    const sf::Color haut = Eclaircir(couleur, 0.7f), hautInterieur = Eclaircir(couleur, 0.6f);
    const sf::Color gaucheHaut = Eclaircir(couleur, 0.45f), gaucheBas = Assombrir(couleur, 0.18f);
    const sf::Color droitHaut = Assombrir(couleur, 0.05f), droitBas = Assombrir(couleur, 0.3f);
    const sf::Color basInterieur = Assombrir(couleur, 0.23f), basBord = Assombrir(couleur, 0.03f);
    const sf::Color faceHaut = Eclaircir(couleur, 0.2f), faceBas = Assombrir(couleur, 0.08f);

    AjouterQuad(sommets, sommet(a.x, a.y, haut), sommet(d.x, a.y, haut), sommet(c.x, b.y, hautInterieur),
                sommet(b.x, b.y, hautInterieur));
    AjouterQuad(sommets, sommet(a.x, a.y, gaucheHaut), sommet(b.x, b.y, gaucheHaut), sommet(b.x, c.y, gaucheBas),
                sommet(a.x, d.y, gaucheBas));
    AjouterQuad(sommets, sommet(d.x, a.y, droitHaut), sommet(d.x, d.y, droitBas), sommet(c.x, c.y, droitBas),
                sommet(c.x, b.y, droitHaut));
    AjouterQuad(sommets, sommet(b.x, c.y, basInterieur), sommet(c.x, c.y, basInterieur), sommet(d.x, d.y, basBord),
                sommet(a.x, d.y, basBord));
    AjouterQuad(sommets, sommet(b.x, b.y, faceHaut), sommet(c.x, b.y, faceHaut), sommet(c.x, c.y, faceBas),
                sommet(b.x, c.y, faceBas));
}

} // namespace formes
