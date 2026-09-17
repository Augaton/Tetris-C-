#include "Rendu.h"

#include "Application.h"
#include "Texte.h"
#include "Touches.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <string>

namespace {

sf::Vector2f Vers(cst::Point p) {
    return {p.x, p.y};
}

sf::Vector2f PositionCase(int x, int y) {
    return {cst::PLATEAU.x + static_cast<float>(cst::TUILE * x), cst::PLATEAU.y + static_cast<float>(cst::TUILE * y)};
}

const sf::Uint32 SIGNE_FOIS = 0xD7; // « × »

// Zone du panneau « Command » de FondPrincipal.png où la liste des touches est redessinée
const sf::FloatRect ZONE_COMMANDES(630.f, 382.f, 222.f, 116.f);

} // namespace

Rendu::Rendu(const sf::Texture& tuiles, const sf::Texture& fondTexture, const sf::Font& police)
    : tuiles(tuiles), police(police), fond(fondTexture) {
    // Capacité réservée une fois : plus de réallocation pendant la partie
    sommets.resize(4 * (cst::LARGEUR * cst::HAUTEUR + 16));
    sommets.clear();
    formes.resize(6 * 24 * 5);
    formes.clear();

    auto preparer = [&](sf::Text& t) {
        t.setFont(police);
        t.setFillColor(sf::Color::White);
        t.setStyle(sf::Text::Bold);
    };
    for (Nombre* n : {&score, &lignes, &niveau}) preparer(n->texte);
    for (sf::Text& t : texteCommandes) preparer(t);
    preparer(texteCombo);
    preparer(texteComboLabel);
    texteComboLabel.setString("COMBO");
    texteComboLabel.setLetterSpacing(2.f);

    limite.setSize({static_cast<float>(cst::TUILE * cst::LARGEUR), 2.f});
    limite.setFillColor(sf::Color(255, 0, 0, 150));
    limite.setPosition(PositionCase(0, cst::LIGNES_ZONE_LIMITE));

    // Le fond contient les touches d'origine : on les masque pour afficher la configuration réelle
    masqueCommandes.setSize({ZONE_COMMANDES.width, ZONE_COMMANDES.height});
    masqueCommandes.setPosition(ZONE_COMMANDES.left, ZONE_COMMANDES.top);
    masqueCommandes.setFillColor(COULEUR_PANNEAU);
}

void Rendu::PrechargerGlyphes(float echelle) {
    echellePrechargee = echelle;
    static const std::string ascii = "0123456789+ !ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::basic_string<sf::Uint32> caracteres(ascii.begin(), ascii.end());
    caracteres.push_back(SIGNE_FOIS);

    const float contour = 2.f * echelle;
    // Tailles logiques des textes de la partie : nombres, combo, textes flottants
    const struct {
        unsigned taille;
        bool avecContour;
    } styles[] = {{20, false}, {11, false}, {18, false}, {18, true}, {22, true}, {24, true}, {28, true}, {30, true}};

    for (const auto& style : styles) {
        const unsigned taille =
            std::max(1u, static_cast<unsigned>(std::lround(static_cast<float>(style.taille) * echelle)));
        for (sf::Uint32 c : caracteres) {
            police.getGlyph(c, taille, true, 0.f);
            if (style.avecContour) police.getGlyph(c, taille, true, contour);
        }
    }
}

void Rendu::AjouterRectangleArrondi(const sf::Transform& transformation, sf::FloatRect zone, float rayon,
                                    sf::Color couleur) {
    if (zone.width <= 0.f || zone.height <= 0.f || couleur.a == 0) return;
    rayon = std::min({rayon, zone.width / 2.f, zone.height / 2.f});

    // Éventail de triangles autour du centre : 4 arcs de SEGMENTS segments chacun
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
            const float angle = (-90.f + 90.f * (static_cast<float>(arc) + static_cast<float>(i) / SEGMENTS)) * 3.14159265f / 180.f;
            const sf::Vector2f point = transformation.transformPoint(centresArcs[arc] + sf::Vector2f(std::cos(angle), std::sin(angle)) * rayon);
            if (arc == 0 && i == 0) {
                premier = point;
            } else {
                formes.append(sf::Vertex(centre, couleur));
                formes.append(sf::Vertex(precedent, couleur));
                formes.append(sf::Vertex(point, couleur));
            }
            precedent = point;
        }
    }
    formes.append(sf::Vertex(centre, couleur));
    formes.append(sf::Vertex(precedent, couleur));
    formes.append(sf::Vertex(premier, couleur));
}

void Rendu::AjouterTuile(int couleur, sf::Vector2f pos, sf::Color teinte) {
    couleur = std::clamp(couleur, 0, 7); // jamais de coordonnées hors de la texture
    const float t = static_cast<float>(cst::TUILE);
    const float u = t * static_cast<float>(couleur);
    sommets.append(sf::Vertex(pos, teinte, {u, 0.f}));
    sommets.append(sf::Vertex({pos.x + t, pos.y}, teinte, {u + t, 0.f}));
    sommets.append(sf::Vertex({pos.x + t, pos.y + t}, teinte, {u + t, t}));
    sommets.append(sf::Vertex({pos.x, pos.y + t}, teinte, {u, t}));
}

void Rendu::AjouterApercu(std::optional<TypePiece> type, cst::Point centre, sf::Color teinte) {
    if (!type) return;

    // Centre la pièce dans son cadre d'après ses cases réelles
    const Cases cases = piece::Forme(*type, 0);
    int minX = INT_MAX, maxX = INT_MIN, minY = INT_MAX, maxY = INT_MIN;
    for (const Case& c : cases) {
        minX = std::min(minX, c.x);
        maxX = std::max(maxX, c.x);
        minY = std::min(minY, c.y);
        maxY = std::max(maxY, c.y);
    }
    const float t = static_cast<float>(cst::TUILE);
    const float origineX = centre.x - static_cast<float>(maxX - minX + 1) * t / 2.f;
    const float origineY = centre.y - static_cast<float>(maxY - minY + 1) * t / 2.f;

    for (const Case& c : cases) {
        AjouterTuile(piece::Couleur(*type),
                     {origineX + static_cast<float>(c.x - minX) * t, origineY + static_cast<float>(c.y - minY) * t},
                     teinte);
    }
}

void Rendu::DessinerNombre(sf::RenderTarget& cible, Nombre& nombre, long long valeur, cst::Point centre,
                           float echelle) {
    if (valeur != nombre.valeur || echelle != nombre.echelle) {
        nombre.valeur = valeur;
        nombre.echelle = echelle;
        nombre.texte.setString(std::to_string(valeur));
        PlacerTexteBorne(nombre.texte, 20, Vers(centre), echelle, 160.f);
    }
    cible.draw(nombre.texte);
}

void Rendu::DessinerCommandes(sf::RenderTarget& cible, const Reglages& reglages, float echelle) {
    cible.draw(masqueCommandes);

    if (reglages.touches != touchesAffichees || echelle != echelleCommandes) {
        touchesAffichees = reglages.touches;
        echelleCommandes = echelle;

        auto principale = [&](Action action) { return touches::Nom(reglages.Touches(action)[0]); };
        const std::array<sf::String, 5> lignesTexte = {
            Utf8("Déplacer : ") + principale(Action::Gauche) + " " + principale(Action::Droite),
            Utf8("Descendre : ") + principale(Action::DescenteDouce) + Utf8("   Chute : ") + principale(Action::ChuteRapide),
            Utf8("Tourner : ") + principale(Action::TournerHoraire) + " / " + principale(Action::TournerAntiHoraire),
            Utf8("Garder : ") + principale(Action::Garder),
            Utf8("Pause : ") + principale(Action::Pause) + Utf8("   Abandon : ") + principale(Action::Abandonner),
        };

        const float hauteurLigne = ZONE_COMMANDES.height / static_cast<float>(texteCommandes.size());
        for (size_t i = 0; i < texteCommandes.size(); i++) {
            texteCommandes[i].setString(lignesTexte[i]);
            const sf::Vector2f centre(ZONE_COMMANDES.left + ZONE_COMMANDES.width / 2.f,
                                      ZONE_COMMANDES.top + hauteurLigne * (static_cast<float>(i) + 0.5f));
            PlacerTexteBorne(texteCommandes[i], 14, centre, echelle, ZONE_COMMANDES.width - 8.f);
        }
    }
    for (const sf::Text& t : texteCommandes) cible.draw(t);
}

void Rendu::Dessiner(sf::RenderTarget& cible, const Jeu& jeu, float temps, float echelle, const Reglages& reglages,
                     Effets& effets) {
    const float dt = std::clamp(temps - dernierTemps, 0.f, 0.1f);
    dernierTemps = temps;
    if (echelle != echellePrechargee) PrechargerGlyphes(echelle);

    cible.draw(fond);

    // Le plateau et ses effets tremblent ensemble ; le reste de l'interface reste fixe
    sf::RenderStates plateau;
    plateau.transform.translate(effets.Secousse());
    DessinerLimite(cible, jeu, temps, plateau);

    sommets.clear();

    const Jeu::Grille& grille = jeu.Plateau();
    for (int y = 0; y < cst::HAUTEUR; y++)
        for (int x = 0; x < cst::LARGEUR; x++)
            if (grille[y][x] != 0) AjouterTuile(grille[y][x], PositionCase(x, y));

    if (!jeu.Perdu()) {
        // Mouvements fluides : la pièce affichée rattrape sa case en ~0,1 s. Nouvelle pièce ou grand saut
        // (répétition instantanée, garde) : placement direct pour ne jamais donner d'impression de retard.
        const sf::Vector2f position(static_cast<float>(jeu.PieceX()), static_cast<float>(jeu.PieceY()));
        const sf::Vector2f ecart = position - positionAffichee;
        if (!reglages.mouvementsFluides || &jeu != jeuSuivi || jeu.NumeroPiece() != numeroSuivi ||
            std::abs(ecart.x) > 2.f || std::abs(ecart.y) > 2.f)
            positionAffichee = position;
        else
            positionAffichee += ecart * (1.f - std::exp(-40.f * dt));
        jeuSuivi = &jeu;
        numeroSuivi = jeu.NumeroPiece();
        const sf::Vector2f decalage = (positionAffichee - position) * static_cast<float>(cst::TUILE);

        const int couleur = piece::Couleur(jeu.PieceActive());
        if (reglages.fantome) {
            // Fantôme qui respire légèrement ; il suit la pièce horizontalement
            const auto alpha = static_cast<sf::Uint8>(90.f + (reglages.effets ? 25.f * std::sin(temps * 5.f) : 0.f));
            for (const Case& c : jeu.CasesFantome())
                AjouterTuile(couleur, PositionCase(c.x, c.y) + sf::Vector2f(decalage.x, 0.f), sf::Color(255, 255, 255, alpha));
        }

        // La pièce s'assombrit pendant le délai de verrouillage
        const auto luminosite = static_cast<sf::Uint8>(255.f - 100.f * jeu.ProgressionVerrouillage());
        const sf::Color teinte(luminosite, luminosite, luminosite);
        for (const Case& c : jeu.CasesPiece()) AjouterTuile(couleur, PositionCase(c.x, c.y) + decalage, teinte);
    }

    plateau.texture = &tuiles;
    cible.draw(sommets, plateau);
    plateau.texture = nullptr;
    effets.DessinerPlateau(cible, plateau);

    sommets.clear();
    AjouterApercu(jeu.PieceSuivante(), cst::APERCU_SUIVANT, sf::Color::White);
    AjouterApercu(jeu.PieceGardee(), cst::APERCU_GARDE,
                  jeu.GardeUtilisee() ? sf::Color(120, 120, 120) : sf::Color::White);
    cible.draw(sommets, &tuiles);

    // Le score affiché rattrape le vrai score en quelques images
    if (!reglages.effets || static_cast<double>(jeu.Score()) < scoreAffiche)
        scoreAffiche = static_cast<double>(jeu.Score());
    else
        scoreAffiche += (static_cast<double>(jeu.Score()) - scoreAffiche) * std::min(1.f, dt * 12.f);
    const long long scoreArrondi =
        jeu.Score() - scoreAffiche < 1.0 ? jeu.Score() : static_cast<long long>(scoreAffiche);

    // Doré dès que le meilleur score est battu
    score.texte.setFillColor(record > 0 && jeu.Score() > record ? sf::Color(255, 204, 0) : sf::Color::White);
    DessinerNombre(cible, score, scoreArrondi, cst::TEXTE_SCORE, echelle);
    DessinerNombre(cible, lignes, jeu.Lignes(), cst::TEXTE_LIGNES, echelle);
    DessinerNombre(cible, niveau, jeu.Niveau(), cst::TEXTE_NIVEAU, echelle);
    DessinerCommandes(cible, reglages, echelle);

    DessinerCombo(cible, jeu, temps, dt, echelle);
    effets.DessinerTextes(cible, echelle);
}

void Rendu::DessinerLimite(sf::RenderTarget& cible, const Jeu& jeu, float temps, const sf::RenderStates& etats) {
    // La ligne limite s'intensifie et clignote quand la pile s'en approche (4 lignes ou moins)
    int plusHaute = cst::HAUTEUR;
    for (int y = 0; y < cst::HAUTEUR && plusHaute == cst::HAUTEUR; y++)
        for (int valeur : jeu.Plateau()[y])
            if (valeur != 0) {
                plusHaute = y;
                break;
            }

    const float danger = std::clamp(static_cast<float>(cst::LIGNES_ZONE_LIMITE + 4 - plusHaute) / 4.f, 0.f, 1.f);
    const float pulsation = 0.5f + 0.5f * std::sin(temps * 9.f);
    limite.setFillColor(sf::Color(255, static_cast<sf::Uint8>(40.f * (1.f - danger)), 0,
                                  static_cast<sf::Uint8>(150.f + 105.f * danger * pulsation)));
    const float epaisseur = 2.f + 2.f * danger;
    if (limite.getSize().y != epaisseur) {
        limite.setSize({static_cast<float>(cst::TUILE * cst::LARGEUR), epaisseur});
        limite.setOrigin(0.f, (epaisseur - 2.f) / 2.f);
    }
    cible.draw(limite, etats);
}

namespace {

// Couleur du badge selon le palier (couleurs des tuiles, pour rester cohérent avec le jeu)
sf::Color CouleurCombo(int combo) {
    if (combo >= 8) return {230, 80, 200};
    if (combo >= 5) return {235, 125, 36};
    if (combo >= 3) return {244, 200, 36};
    if (combo >= 2) return {102, 191, 41};
    return {48, 190, 229};
}

sf::Color Attenuer(sf::Color couleur, float facteur) {
    couleur.a = static_cast<sf::Uint8>(static_cast<float>(couleur.a) * std::clamp(facteur, 0.f, 1.f));
    return couleur;
}

} // namespace

void Rendu::DessinerCombo(sf::RenderTarget& cible, const Jeu& jeu, float temps, float dt, float echelle) {
    const int combo = jeu.Combo();

    if (combo > 0) {
        if (combo != comboAffiche) {
            comboAffiche = combo;
            tempsCombo = 0.f;
            texteCombo.setString(sf::String(SIGNE_FOIS) + std::to_string(combo));
        }
        disparitionCombo = 0.f;
    } else if (comboAffiche > 0) {
        disparitionCombo += dt;
        if (disparitionCombo >= cst::COMBO_DISPARITION_S) comboAffiche = 0;
    }
    if (comboAffiche <= 0) return;
    tempsCombo += dt;

    const float ratio = combo > 0 ? std::clamp(jeu.ComboRestant() / cst::COMBO_DUREE_S, 0.f, 1.f) : 0.f;
    const float fondu = 1.f - disparitionCombo / cst::COMBO_DISPARITION_S;

    // Apparition avec rebond amorti, disparition en rétrécissant
    const float rebond = 0.3f * std::exp(-10.f * tempsCombo) * std::cos(18.f * tempsCombo);
    const float zoom = (1.f + rebond) * (0.6f + 0.4f * fondu);

    // Temps presque écoulé : le contour clignote
    const float alerte = (combo > 0 && ratio < 0.25f) ? 0.45f + 0.55f * std::abs(std::sin(temps * 14.f)) : 1.f;

    const sf::Color couleur = CouleurCombo(comboAffiche);
    const sf::Vector2f centre = Vers(cst::BADGE_COMBO);
    const float largeur = 150.f, hauteur = 30.f, rayon = hauteur / 2.f;

    sf::Transform transformation;
    transformation.translate(centre).scale(zoom, zoom);

    formes.clear();
    const sf::FloatRect badge(-largeur / 2.f, -hauteur / 2.f, largeur, hauteur);
    auto agrandi = [&](float marge) {
        return sf::FloatRect(badge.left - marge, badge.top - marge, badge.width + 2.f * marge, badge.height + 2.f * marge);
    };

    AjouterRectangleArrondi(transformation, agrandi(6.f), rayon + 6.f, Attenuer(sf::Color(couleur.r, couleur.g, couleur.b, 40), fondu * alerte));
    AjouterRectangleArrondi(transformation, agrandi(2.f), rayon + 2.f, Attenuer(couleur, fondu * alerte));
    AjouterRectangleArrondi(transformation, badge, rayon, Attenuer(sf::Color(18, 18, 18, 235), fondu));
    // Temps restant : remplissage qui se vide de droite à gauche
    AjouterRectangleArrondi(transformation, {badge.left, badge.top, badge.width * ratio, badge.height}, rayon,
                            Attenuer(sf::Color(couleur.r, couleur.g, couleur.b, 70), fondu));
    // Éclair blanc à chaque nouveau palier
    AjouterRectangleArrondi(transformation, badge, rayon,
                            Attenuer(sf::Color(255, 255, 255, 200), std::exp(-14.f * tempsCombo) * fondu));
    cible.draw(formes);

    const auto alpha = static_cast<sf::Uint8>(255.f * fondu);
    texteComboLabel.setFillColor(sf::Color(220, 220, 220, alpha));
    PlacerTexte(texteComboLabel, 11, transformation.transformPoint(-30.f, 0.f), echelle, zoom);
    cible.draw(texteComboLabel);

    texteCombo.setFillColor(sf::Color(couleur.r, couleur.g, couleur.b, alpha));
    PlacerTexte(texteCombo, 18, transformation.transformPoint(40.f, 0.f), echelle, zoom);
    cible.draw(texteCombo);
}
