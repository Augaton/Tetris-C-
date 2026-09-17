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

// Zone du panneau « Command » de FondPrincipal.png où la liste des touches est redessinée
const sf::FloatRect ZONE_COMMANDES(630.f, 382.f, 222.f, 116.f);

} // namespace

Rendu::Rendu(const sf::Texture& tuiles, const sf::Texture& fondTexture, const sf::Font& police)
    : tuiles(tuiles), fond(fondTexture) {
    auto preparer = [&](sf::Text& t) {
        t.setFont(police);
        t.setFillColor(sf::Color::White);
        t.setStyle(sf::Text::Bold);
    };
    for (Nombre* n : {&score, &lignes, &niveau}) preparer(n->texte);
    for (sf::Text& t : texteCommandes) preparer(t);
    preparer(texteCombo);
    texteCombo.setOutlineColor(sf::Color::Black);

    limite.setSize({static_cast<float>(cst::TUILE * cst::LARGEUR), 2.f});
    limite.setFillColor(sf::Color(255, 0, 0, 150));
    limite.setPosition(PositionCase(0, cst::LIGNES_ZONE_LIMITE));

    // Le fond contient les touches d'origine : on les masque pour afficher la configuration réelle
    masqueCommandes.setSize({ZONE_COMMANDES.width, ZONE_COMMANDES.height});
    masqueCommandes.setPosition(ZONE_COMMANDES.left, ZONE_COMMANDES.top);
    masqueCommandes.setFillColor(COULEUR_PANNEAU);
}

void Rendu::AjouterTuile(int couleur, sf::Vector2f pos, sf::Color teinte) {
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

void Rendu::Dessiner(sf::RenderTarget& cible, const Jeu& jeu, float temps, float echelle, const Reglages& reglages) {
    cible.draw(fond);
    cible.draw(limite);

    sommets.clear();

    const Jeu::Grille& grille = jeu.Plateau();
    for (int y = 0; y < cst::HAUTEUR; y++)
        for (int x = 0; x < cst::LARGEUR; x++)
            if (grille[y][x] != 0) AjouterTuile(grille[y][x], PositionCase(x, y));

    if (!jeu.Perdu()) {
        const int couleur = piece::Couleur(jeu.PieceActive());
        if (reglages.fantome)
            for (const Case& c : jeu.CasesFantome()) AjouterTuile(couleur, PositionCase(c.x, c.y), sf::Color(255, 255, 255, 100));

        // La pièce s'assombrit pendant le délai de verrouillage
        const auto luminosite = static_cast<sf::Uint8>(255.f - 100.f * jeu.ProgressionVerrouillage());
        const sf::Color teinte(luminosite, luminosite, luminosite);
        for (const Case& c : jeu.CasesPiece()) AjouterTuile(couleur, PositionCase(c.x, c.y), teinte);
    }

    AjouterApercu(jeu.PieceSuivante(), cst::APERCU_SUIVANT, sf::Color::White);
    AjouterApercu(jeu.PieceGardee(), cst::APERCU_GARDE,
                  jeu.GardeUtilisee() ? sf::Color(120, 120, 120) : sf::Color::White);

    cible.draw(sommets, &tuiles);

    DessinerNombre(cible, score, jeu.Score(), cst::TEXTE_SCORE, echelle);
    DessinerNombre(cible, lignes, jeu.Lignes(), cst::TEXTE_LIGNES, echelle);
    DessinerNombre(cible, niveau, jeu.Niveau(), cst::TEXTE_NIVEAU, echelle);
    DessinerCommandes(cible, reglages, echelle);

    DessinerCombo(cible, jeu, temps, echelle);
}

void Rendu::DessinerCombo(sf::RenderTarget& cible, const Jeu& jeu, float temps, float echelle) {
    const int combo = jeu.Combo();
    if (combo <= 0) return;

    const float angle = std::sin(temps * 4.f) * 5.f;
    const float pulsation = 1.f + std::sin(temps * 10.f) * 0.1f;

    if (combo != comboAffiche) {
        comboAffiche = combo;
        texteCombo.setString("COMBO X" + std::to_string(combo));
    }

    if (combo >= 8)      texteCombo.setFillColor(sf::Color(255, 0, 255));
    else if (combo >= 5) texteCombo.setFillColor(sf::Color(255, 50, 50));
    else if (combo >= 3) texteCombo.setFillColor(sf::Color(255, 165, 0));
    else                 texteCombo.setFillColor(sf::Color::Cyan);

    // Contour en pixels réels : ramené à 2 unités logiques
    texteCombo.setOutlineThickness(2.f * echelle);
    PlacerTexte(texteCombo, 25, Vers(cst::TEXTE_COMBO), echelle, pulsation);
    texteCombo.setRotation(angle);
    cible.draw(texteCombo);

    // Barre de temps restant
    const float ratio = std::clamp(jeu.ComboRestant() / cst::COMBO_DUREE_S, 0.f, 1.f);
    const float largeur = 140.f;
    const float hauteur = 8.f;
    const float radians = angle * 3.14159f / 180.f;
    const sf::Vector2f centre = Vers(cst::BARRE_COMBO);

    sf::RectangleShape barre(sf::Vector2f(largeur, hauteur));
    barre.setOrigin(largeur / 2.f, hauteur / 2.f);
    barre.setPosition(centre);
    barre.setRotation(angle);
    barre.setFillColor(sf::Color(0, 0, 0, 150));
    barre.setOutlineThickness(1.5f);
    barre.setOutlineColor(sf::Color(255, 255, 255, 80));
    cible.draw(barre);

    if (ratio <= 0.01f) return;

    // Bord gauche de la barre, en tenant compte de la rotation
    const sf::Vector2f gauche(centre.x - (largeur / 2.f) * std::cos(radians),
                              centre.y - (largeur / 2.f) * std::sin(radians));

    sf::Color couleur;
    if (ratio > 0.5f)      couleur = sf::Color(0, 255, 150);
    else if (ratio > 0.2f) couleur = sf::Color(255, 200, 0);
    else                   couleur = sf::Color(255, 50, 50);

    sf::RectangleShape remplissage(sf::Vector2f(largeur * ratio, hauteur));
    remplissage.setOrigin(0.f, hauteur / 2.f);
    remplissage.setPosition(gauche);
    remplissage.setRotation(angle);
    remplissage.setFillColor(couleur);
    cible.draw(remplissage);

    sf::RectangleShape brillance(sf::Vector2f(largeur * ratio, hauteur / 2.f));
    brillance.setOrigin(0.f, hauteur / 4.f);
    brillance.setPosition(gauche);
    brillance.setRotation(angle);
    brillance.setFillColor(sf::Color(255, 255, 255, 50));
    cible.draw(brillance);
}
