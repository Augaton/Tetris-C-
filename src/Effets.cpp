#include "Effets.h"

#include "Constantes.h"
#include "Palette.h"
#include "Texte.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace {

const float T = static_cast<float>(cst::TUILE);

sf::Vector2f CoinCase(int x, int y) {
    return {cst::PLATEAU.x + T * static_cast<float>(x), cst::PLATEAU.y + T * static_cast<float>(y)};
}

sf::Color AvecAlpha(sf::Color couleur, float alpha) {
    couleur.a = static_cast<sf::Uint8>(std::clamp(alpha, 0.f, 1.f) * 255.f);
    return couleur;
}

// Retire les éléments terminés sans décaler le tableau (l'ordre n'a pas d'importance)
template <typename V>
void RetirerFinis(V& elements) {
    for (size_t i = 0; i < elements.size();) {
        if (elements[i].vie >= elements[i].duree) {
            elements[i] = elements.back();
            elements.pop_back();
        } else {
            i++;
        }
    }
}

} // namespace

Effets::Effets(const sf::Font& police) {
    particules.reserve(MAX_PARTICULES);
    eclats.reserve(MAX_ECLATS);
    trainees.reserve(MAX_TRAINEES);
    sommets.resize(4 * (MAX_PARTICULES + MAX_ECLATS + MAX_TRAINEES));
    sommets.clear(); // garde la capacité : aucune réallocation en jeu
    for (TexteFlottant& t : textes) {
        t.texte.setFont(police);
        t.texte.setStyle(sf::Text::Bold);
        t.texte.setOutlineColor(sf::Color::Black);
    }
}

float Effets::Aleatoire(float min, float max) {
    return std::uniform_real_distribution<float>(min, max)(rng);
}

void Effets::AjouterParticule(sf::Vector2f position, sf::Vector2f vitesse, sf::Color couleur, float duree, float taille) {
    if (particules.size() >= MAX_PARTICULES) return;
    particules.push_back({position, vitesse, couleur, 0.f, duree, taille});
}

void Effets::AjouterEclat(sf::FloatRect zone, float duree, bool seTasse) {
    if (eclats.size() >= MAX_ECLATS) return;
    eclats.push_back({zone, 0.f, duree, seTasse});
}

void Effets::AfficherTexte(const sf::String& chaine, sf::Vector2f position, unsigned taille, sf::Color couleur,
                           float duree) {
    // Remplace le texte le plus avancé si tous sont occupés
    TexteFlottant* libre = &textes[0];
    for (TexteFlottant& t : textes) {
        if (t.vie >= t.duree) {
            libre = &t;
            break;
        }
        if (t.vie / t.duree > libre->vie / libre->duree) libre = &t;
    }
    libre->texte.setString(chaine);
    libre->texte.setFillColor(couleur);
    libre->depart = position;
    libre->taille = taille;
    libre->vie = 0.f;
    libre->duree = duree;
}

void Effets::Secouer(float amplitude) {
    secousse = std::max(secousse, amplitude);
}

void Effets::Traiter(const std::vector<EvenementJeu>& evenements, const Reglages& reglages) {
    const bool effetsActifs = reglages.effets;
    const bool secoussesActives = reglages.secousses;
    const auto CouleurTuile = [&](int tuile) { return palette::Tuile(tuile, reglages.daltonien); };

    for (const EvenementJeu& e : evenements) {
        switch (e.type) {
            case EvenementJeu::Type::ChuteRapide: {
                if (secoussesActives) Secouer(2.f + std::min(3.f, static_cast<float>(e.distance) * 0.2f));
                if (!effetsActifs || e.distance <= 0) break;

                // Une traînée par colonne, du point de départ jusqu'à la pièce
                const sf::Color couleur = CouleurTuile(e.couleur);
                for (const Case& c : e.cases) {
                    bool plusHaute = true;
                    for (const Case& autre : e.cases)
                        if (autre.x == c.x && autre.y < c.y) plusHaute = false;
                    if (!plusHaute || trainees.size() >= MAX_TRAINEES) continue;

                    const float bas = CoinCase(c.x, c.y).y;
                    trainees.push_back({CoinCase(c.x, c.y).x, bas - T * static_cast<float>(e.distance), bas, couleur,
                                        0.f, 0.18f});
                }
                // Poussière sous les cases les plus basses
                for (const Case& c : e.cases) {
                    bool plusBasse = true;
                    for (const Case& autre : e.cases)
                        if (autre.x == c.x && autre.y > c.y) plusBasse = false;
                    if (!plusBasse) continue;
                    const sf::Vector2f sol = CoinCase(c.x, c.y + 1);
                    for (int i = 0; i < 3; i++)
                        AjouterParticule({sol.x + Aleatoire(0.f, T), sol.y},
                                         {Aleatoire(-60.f, 60.f), Aleatoire(-90.f, -30.f)},
                                         sf::Color(220, 220, 220), Aleatoire(0.2f, 0.35f), 2.f);
                }
                break;
            }

            case EvenementJeu::Type::Verrouillage:
                if (!effetsActifs) break;
                for (const Case& c : e.cases) AjouterEclat({CoinCase(c.x, c.y), {T, T}}, 0.15f, false);
                break;

            case EvenementJeu::Type::Lignes: {
                if (secoussesActives) Secouer(e.nbLignes >= 4 ? 7.f : 1.5f * static_cast<float>(e.nbLignes));
                if (!effetsActifs) break;

                float yMoyen = 0.f;
                for (int i = 0; i < e.nbLignes; i++) {
                    const int y = e.lignes[static_cast<size_t>(i)];
                    const sf::Vector2f coin = CoinCase(0, y);
                    yMoyen += coin.y + T / 2.f;
                    AjouterEclat({coin, {T * static_cast<float>(cst::LARGEUR), T}}, 0.3f, true);

                    // Chaque case éclate en deux particules de sa couleur, projetées depuis le centre
                    for (int x = 0; x < cst::LARGEUR; x++) {
                        const sf::Vector2f centre = CoinCase(x, y) + sf::Vector2f(T / 2.f, T / 2.f);
                        const float direction = (static_cast<float>(x) - 4.5f) / 4.5f;
                        const sf::Color couleur = CouleurTuile(e.contenu[static_cast<size_t>(i)][static_cast<size_t>(x)]);
                        for (int k = 0; k < 2; k++)
                            AjouterParticule(centre,
                                             {direction * Aleatoire(60.f, 220.f) + Aleatoire(-40.f, 40.f),
                                              Aleatoire(-260.f, -60.f)},
                                             couleur, Aleatoire(0.45f, 0.8f), Aleatoire(3.f, 5.f));
                    }
                }
                yMoyen /= static_cast<float>(std::max(1, e.nbLignes));

                const char* NOMS[] = {"", "", "DOUBLE", "TRIPLE", Tr("TETRIS !", "TETRIS!")};
                const std::string points = "+" + FormaterNombre(e.points);
                const sf::Vector2f centre(cst::PLATEAU.x + T * cst::LARGEUR / 2.f, yMoyen);
                if (e.nbLignes >= 2)
                    AfficherTexte(Utf8(std::string(NOMS[std::min(e.nbLignes, 4)]) + "  " + points), centre,
                                  e.nbLignes >= 4 ? 28 : 22, e.nbLignes >= 4 ? sf::Color::Cyan : sf::Color::White, 1.f);
                else
                    AfficherTexte(Utf8(points), centre, 18, sf::Color::White, 0.8f);
                break;
            }

            case EvenementJeu::Type::Nettoyage:
                // Zen : la pile a atteint le haut et vient d'être vidée
                if (secoussesActives) Secouer(5.f);
                if (!effetsActifs) break;
                for (int y = 0; y < cst::HAUTEUR; y++)
                    AjouterEclat({CoinCase(0, y), {T * static_cast<float>(cst::LARGEUR), T}}, 0.45f, true);
                AfficherTexte(TrU("PLATEAU VIDÉ", "BOARD CLEARED"),
                              {cst::PLATEAU.x + T * cst::LARGEUR / 2.f, cst::PLATEAU.y + 180.f}, 24, sf::Color(86, 180, 233), 1.3f);
                break;

            case EvenementJeu::Type::Niveau:
                if (!effetsActifs) break;
                AfficherTexte(Utf8(Tr("NIVEAU ", "LEVEL ") + std::to_string(e.niveau)),
                              {cst::PLATEAU.x + T * cst::LARGEUR / 2.f, cst::PLATEAU.y + 110.f}, 30,
                              sf::Color(255, 204, 0), 1.3f);
                break;
        }
    }
}

void Effets::AnnoncerRecord() {
    AfficherTexte(TrU("NOUVEAU RECORD !", "NEW RECORD!"), {cst::PLATEAU.x + T * cst::LARGEUR / 2.f, cst::PLATEAU.y + 60.f}, 24,
                  sf::Color(255, 204, 0), 1.6f);
}

void Effets::MettreAJour(float dt) {
    for (Particule& p : particules) {
        p.vie += dt;
        p.vitesse.y += 700.f * dt;          // gravité
        p.vitesse *= std::max(0.f, 1.f - 1.5f * dt); // frottement
        p.position += p.vitesse * dt;
    }
    for (Eclat& e : eclats) e.vie += dt;
    for (Trainee& t : trainees) t.vie += dt;
    for (TexteFlottant& t : textes)
        if (t.vie < t.duree) t.vie += dt;

    RetirerFinis(particules);
    RetirerFinis(eclats);
    RetirerFinis(trainees);

    tempsSecousse += dt;
    secousse *= std::exp(-14.f * dt);
    if (secousse < 0.05f) secousse = 0.f;
}

sf::Vector2f Effets::Secousse() const {
    if (secousse <= 0.f) return {};
    return {std::sin(tempsSecousse * 91.f) * secousse, std::cos(tempsSecousse * 73.f) * secousse * 0.6f};
}

void Effets::AjouterQuad(sf::FloatRect zone, sf::Color haut, sf::Color bas) {
    sommets.append(sf::Vertex({zone.left, zone.top}, haut));
    sommets.append(sf::Vertex({zone.left + zone.width, zone.top}, haut));
    sommets.append(sf::Vertex({zone.left + zone.width, zone.top + zone.height}, bas));
    sommets.append(sf::Vertex({zone.left, zone.top + zone.height}, bas));
}

void Effets::DessinerPlateau(sf::RenderTarget& cible, const sf::RenderStates& etats) {
    sommets.clear();

    for (const Trainee& t : trainees) {
        const float restant = 1.f - t.vie / t.duree;
        AjouterQuad({t.x, t.haut, T, t.bas - t.haut}, AvecAlpha(t.couleur, 0.f), AvecAlpha(t.couleur, 0.45f * restant));
    }

    for (const Eclat& e : eclats) {
        const float progression = e.vie / e.duree;
        sf::FloatRect zone = e.zone;
        if (e.seTasse) {
            const float hauteur = zone.height * (1.f - progression * progression);
            zone.top += (zone.height - hauteur) / 2.f;
            zone.height = hauteur;
        }
        const sf::Color blanc = AvecAlpha(sf::Color::White, (e.seTasse ? 0.9f : 0.55f) * (1.f - progression));
        AjouterQuad(zone, blanc, blanc);
    }

    for (const Particule& p : particules) {
        const float restant = 1.f - p.vie / p.duree;
        const float taille = p.taille * (0.4f + 0.6f * restant);
        const sf::Color couleur = AvecAlpha(p.couleur, std::min(1.f, restant * 2.f));
        AjouterQuad({p.position.x - taille / 2.f, p.position.y - taille / 2.f, taille, taille}, couleur, couleur);
    }

    if (sommets.getVertexCount() > 0) cible.draw(sommets, etats);
}

void Effets::DessinerTextes(sf::RenderTarget& cible, float echelle) {
    for (TexteFlottant& t : textes) {
        if (t.vie >= t.duree) continue;
        const float progression = t.vie / t.duree;

        // Apparition avec un léger rebond, montée, puis fondu sur la fin
        const float apparition = std::min(1.f, t.vie / 0.12f);
        const float zoom = 1.f + 0.35f * (1.f - apparition);
        const float alpha = progression < 0.6f ? 1.f : 1.f - (progression - 0.6f) / 0.4f;

        sf::Color couleur = t.texte.getFillColor();
        couleur.a = static_cast<sf::Uint8>(255.f * alpha);
        t.texte.setFillColor(couleur);
        t.texte.setOutlineColor(sf::Color(0, 0, 0, couleur.a));
        t.texte.setOutlineThickness(2.f * echelle);

        PlacerTexte(t.texte, t.taille, {t.depart.x, t.depart.y - 40.f * progression}, echelle, zoom);
        cible.draw(t.texte);
    }
}
