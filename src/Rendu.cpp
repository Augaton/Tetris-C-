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
    : tuiles(tuiles), police(police), fond(fondTexture) {
    // Capacité réservée une fois : plus de réallocation pendant la partie
    sommets.resize(4 * (cst::LARGEUR * cst::HAUTEUR + 16));
    sommets.clear();
    formes.resize(4 * 8);
    formes.clear();

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

void Rendu::PrechargerGlyphes(float echelle) {
    echellePrechargee = echelle;
    static const std::string caracteres = "0123456789+ !ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    const float contour = 2.f * echelle;
    // Tailles logiques des textes de la partie : nombres, combo, textes flottants
    const struct {
        unsigned taille;
        bool avecContour;
    } styles[] = {{20, false}, {25, true}, {18, true}, {22, true}, {28, true}, {30, true}};

    for (const auto& style : styles) {
        const unsigned taille =
            std::max(1u, static_cast<unsigned>(std::lround(static_cast<float>(style.taille) * echelle)));
        for (char c : caracteres) {
            police.getGlyph(static_cast<sf::Uint32>(c), taille, true, 0.f);
            if (style.avecContour) police.getGlyph(static_cast<sf::Uint32>(c), taille, true, contour);
        }
    }
}

void Rendu::AjouterRectangle(const sf::Transform& transformation, sf::FloatRect zone, sf::Color couleur) {
    const sf::Vector2f coins[] = {
        {zone.left, zone.top},
        {zone.left + zone.width, zone.top},
        {zone.left + zone.width, zone.top + zone.height},
        {zone.left, zone.top + zone.height},
    };
    for (const sf::Vector2f& coin : coins) formes.append(sf::Vertex(transformation.transformPoint(coin), couleur));
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
    cible.draw(limite, plateau);

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

    DessinerNombre(cible, score, scoreArrondi, cst::TEXTE_SCORE, echelle);
    DessinerNombre(cible, lignes, jeu.Lignes(), cst::TEXTE_LIGNES, echelle);
    DessinerNombre(cible, niveau, jeu.Niveau(), cst::TEXTE_NIVEAU, echelle);
    DessinerCommandes(cible, reglages, echelle);

    DessinerCombo(cible, jeu, temps, echelle);
    effets.DessinerTextes(cible, echelle);
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

    // Barre de temps restant : quelques quads tournés, un seul appel de dessin
    const float ratio = std::clamp(jeu.ComboRestant() / cst::COMBO_DUREE_S, 0.f, 1.f);
    const float largeur = 140.f;
    const float hauteur = 8.f;
    const float bord = 1.5f;

    sf::Transform transformation;
    transformation.translate(Vers(cst::BARRE_COMBO)).rotate(angle);

    formes.clear();
    const sf::Color couleurBord(255, 255, 255, 80);
    const float gauche = -largeur / 2.f, haut = -hauteur / 2.f;
    AjouterRectangle(transformation, {gauche, haut, largeur, hauteur}, sf::Color(0, 0, 0, 150));
    AjouterRectangle(transformation, {gauche - bord, haut - bord, largeur + 2.f * bord, bord}, couleurBord);
    AjouterRectangle(transformation, {gauche - bord, haut + hauteur, largeur + 2.f * bord, bord}, couleurBord);
    AjouterRectangle(transformation, {gauche - bord, haut, bord, hauteur}, couleurBord);
    AjouterRectangle(transformation, {gauche + largeur, haut, bord, hauteur}, couleurBord);

    if (ratio > 0.01f) {
        sf::Color couleur;
        if (ratio > 0.5f)      couleur = sf::Color(0, 255, 150);
        else if (ratio > 0.2f) couleur = sf::Color(255, 200, 0);
        else                   couleur = sf::Color(255, 50, 50);

        AjouterRectangle(transformation, {gauche, haut, largeur * ratio, hauteur}, couleur);
        AjouterRectangle(transformation, {gauche, -hauteur / 4.f, largeur * ratio, hauteur / 2.f},
                         sf::Color(255, 255, 255, 50));
    }
    cible.draw(formes);
}
