#include "Rendu.h"

#include "Application.h"
#include "Formes.h"
#include "Texte.h"
#include "Touches.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <ranges>
#include <string>
#include <vector>

namespace {

sf::Vector2f Vers(cst::Point p) {
    return {p.x, p.y};
}

sf::Vector2f PositionCase(int x, int y) {
    return {cst::PLATEAU.x + static_cast<float>(cst::TUILE * x), cst::PLATEAU.y + static_cast<float>(cst::TUILE * y)};
}

constexpr char32_t SIGNE_FOIS = U'×';

// Zone du panneau « Command » de FondPrincipal.png où la liste des touches est redessinée
constexpr sf::FloatRect ZONE_COMMANDES({630.f, 382.f}, {222.f, 116.f});

// Cadres des titres de FondPrincipal.png (bordure comprise) : Gardé, Suivant, Lignes, Niveau
constexpr std::array<sf::FloatRect, 4> CADRES_TITRES = {{
    {{102.f, 87.f}, {114.f, 50.f}},
    {{678.f, 25.f}, {114.f, 50.f}},
    {{86.f, 277.f}, {143.f, 39.f}},
    {{87.f, 402.f}, {143.f, 39.f}},
}};

// Motifs d'accessibilité, en coordonnées locales d'une tuile de 18 px
using Quad = std::array<sf::Vector2f, 4>;

constexpr Quad Rect(float x, float y, float l, float h) {
    return {{{x, y}, {x + l, y}, {x + l, y + h}, {x, y + h}}};
}

const std::vector<Quad>& Motif(int tuile) {
    static const std::array<std::vector<Quad>, 8> motifs = {{
        {},
        {Rect(6, 6, 6, 6)},                                                            // T : carré plein
        {Rect(4, 8, 10, 2)},                                                           // Z : barre horizontale
        {Rect(8, 4, 2, 10)},                                                           // S : barre verticale
        {Rect(4, 4, 10, 2), Rect(4, 12, 10, 2), Rect(4, 6, 2, 6), Rect(12, 6, 2, 6)}, // O : carré creux
        {Rect(4, 7.5f, 3, 3), Rect(11, 7.5f, 3, 3)},                                   // I : deux points
        {Rect(4, 8, 10, 2), Rect(8, 4, 2, 4), Rect(8, 10, 2, 4)},                      // L : plus
        {Quad{{{4, 5}, {5, 4}, {14, 13}, {13, 14}}}, Quad{{{13, 4}, {14, 5}, {5, 14}, {4, 13}}}}, // J : croix
    }};
    return motifs[static_cast<size_t>(std::clamp(tuile, 0, 7))];
}

} // namespace

Rendu::Rendu(const sf::Texture& textureTuiles, const sf::Texture& textureDaltonien, const sf::Texture& textureFond,
             const sf::Font& policeTexte)
    : tuiles(textureTuiles), tuilesDaltonien(textureDaltonien), police(policeTexte), fond(textureFond) {
    // Capacité réservée une fois : plus de réallocation pendant la partie (6 sommets par tuile)
    sommets.resize(6 * (cst::LARGEUR * cst::HAUTEUR + 16));
    sommets.clear();
    formes.resize(6 * 24 * 5);
    formes.clear();
    motifs.resize(6 * 4 * (cst::LARGEUR * cst::HAUTEUR + 16));
    motifs.clear();

    auto preparer = [](sf::Text& t) {
        t.setFillColor(sf::Color::White);
        t.setStyle(sf::Text::Bold);
    };
    for (TexteCache* c : {&score, &lignes, &niveau, &piedMode}) preparer(c->texte);
    for (TexteCache& c : etiquettes) preparer(c.texte);
    for (sf::Text& t : texteCommandes) preparer(t);
    preparer(texteCombo);
    preparer(texteComboLabel);
    texteComboLabel.setLetterSpacing(2.f);

    limite.setSize({static_cast<float>(cst::TUILE * cst::LARGEUR), 2.f});
    limite.setFillColor(sf::Color(255, 0, 0, 150));
    limite.setPosition(PositionCase(0, cst::LIGNES_ZONE_LIMITE));

    // Le fond contient les touches d'origine : on les masque pour afficher la configuration réelle
    masqueCommandes.setSize(ZONE_COMMANDES.size);
    masqueCommandes.setPosition(ZONE_COMMANDES.position);
    masqueCommandes.setFillColor(COULEUR_PANNEAU);
}

void Rendu::PrechargerGlyphes(float echelle) {
    echellePrechargee = echelle;
    // Avec « × » du combo et l'espace insécable des nombres
    static const std::u32string caracteres = U"0123456789+ !,.:/-ABCDEFGHIJKLMNOPQRSTUVWXYZ×\xA0";

    const float contour = 2.f * echelle;
    // Tailles logiques des textes de la partie : nombres, combo, textes flottants
    struct Style {
        unsigned taille;
        bool avecContour;
    };
    constexpr std::array<Style, 8> styles = {
        {{20, false}, {11, false}, {18, false}, {18, true}, {22, true}, {24, true}, {28, true}, {30, true}}};

    for (const Style& style : styles) {
        const unsigned taille =
            std::max(1u, static_cast<unsigned>(std::lround(static_cast<float>(style.taille) * echelle)));
        for (const char32_t c : caracteres) {
            // Seul l'effet compte : le glyphe est rastérisé dans la texture de la police
            static_cast<void>(police.getGlyph(c, taille, true, 0.f));
            if (style.avecContour) static_cast<void>(police.getGlyph(c, taille, true, contour));
        }
    }
}

void Rendu::AjouterRectangleArrondi(const sf::Transform& transformation, sf::FloatRect zone, float rayon,
                                    sf::Color couleur) {
    formes::AjouterRectangleArrondi(formes, zone, rayon, couleur, transformation);
}

void Rendu::AjouterTuile(int couleur, sf::Vector2f pos, sf::Color teinte, bool avecMotif) {
    couleur = std::clamp(couleur, 0, 7); // jamais de coordonnées hors de la texture
    const float t = static_cast<float>(cst::TUILE);
    const sf::Vector2f origineTexture(t * static_cast<float>(couleur), 0.f);
    const auto coin = [&](float dx, float dy) {
        return sf::Vertex{pos + sf::Vector2f(dx, dy), teinte, origineTexture + sf::Vector2f(dx, dy)};
    };
    formes::AjouterQuad(sommets, coin(0.f, 0.f), coin(t, 0.f), coin(t, t), coin(0.f, t));

    if (!motifsActifs || !avecMotif) return;
    const sf::Color encre(0, 0, 0, static_cast<std::uint8_t>(teinte.a * 120 / 255));
    for (const Quad& q : Motif(couleur))
        formes::AjouterQuad(motifs, {pos + q[0], encre}, {pos + q[1], encre}, {pos + q[2], encre}, {pos + q[3], encre});
}

void Rendu::AjouterApercu(std::optional<TypePiece> type, cst::Point centre, sf::Color teinte) {
    if (!type) return;

    // Centre la pièce dans son cadre d'après ses cases réelles
    const Cases cases = piece::Forme(*type, 0);
    const auto [minX, maxX] = std::ranges::minmax(cases | std::views::transform(&Case::x));
    const auto [minY, maxY] = std::ranges::minmax(cases | std::views::transform(&Case::y));
    const float t = static_cast<float>(cst::TUILE);
    const float origineX = centre.x - static_cast<float>(maxX - minX + 1) * t / 2.f;
    const float origineY = centre.y - static_cast<float>(maxY - minY + 1) * t / 2.f;

    for (const Case& c : cases) {
        AjouterTuile(piece::Couleur(*type),
                     {origineX + static_cast<float>(c.x - minX) * t, origineY + static_cast<float>(c.y - minY) * t},
                     teinte);
    }
}

void Rendu::DessinerTexte(sf::RenderTarget& cible, TexteCache& cache, const sf::String& chaine, unsigned taille,
                          sf::Vector2f centre, float echelle, float largeurMax, sf::Color couleur) {
    if (chaine != cache.chaine || echelle != cache.echelle) {
        cache.chaine = chaine;
        cache.echelle = echelle;
        cache.texte.setString(chaine);
        PlacerTexteBorne(cache.texte, taille, centre, echelle, largeurMax);
    }
    cache.texte.setFillColor(couleur);
    cible.draw(cache.texte);
}

void Rendu::DessinerInfos(sf::RenderTarget& cible, const Jeu& jeu, long long scoreAffichage, float echelle) {
    const ParametresPartie& p = jeu.Parametres();
    const bool anglais = LangueActuelle() == Langue::Anglais;

    // Titres de FondPrincipal.png : traduits en anglais, et « Niveau » remplacé selon le mode
    std::array<sf::String, 4> titres;
    if (anglais) titres = {Utf8("Hold"), Utf8("Next"), Utf8("Lines"), Utf8("Level")};
    if (p.mode == Mode::Sprint || p.mode == Mode::Ultra) titres[3] = TrU("Temps", "Time");
    if (p.mode == Mode::Zen) titres[3] = TrU("Pièces", "Pieces");
    for (size_t i = 0; i < titres.size(); i++) {
        if (titres[i].isEmpty()) continue;
        const sf::FloatRect& c = CADRES_TITRES[i];
        sf::RectangleShape masque(c.size - sf::Vector2f(4.f, 4.f));
        masque.setPosition(c.position + sf::Vector2f(2.f, 2.f));
        masque.setFillColor(COULEUR_PANNEAU);
        cible.draw(masque);
        DessinerTexte(cible, etiquettes[i], titres[i], 21, c.getCenter(), echelle, c.size.x - 12.f);
    }

    // Valeurs des panneaux
    constexpr sf::Color dore(255, 204, 0);
    const bool recordBattu = record > 0 && jeu.Score() > record;
    DessinerTexte(cible, score, Utf8(FormaterNombre(scoreAffichage)), 20, Vers(cst::TEXTE_SCORE), echelle, 160.f,
                  recordBattu ? dore : sf::Color::White);

    const std::string texteLignes = p.mode == Mode::Sprint ? std::format("{} / {}", jeu.Lignes(), p.sprintLignes)
                                                           : FormaterNombre(jeu.Lignes());
    DessinerTexte(cible, lignes, Utf8(texteLignes), 20, Vers(cst::TEXTE_LIGNES), echelle, 160.f);

    std::string texteNiveau;
    sf::Color couleurNiveau = sf::Color::White;
    switch (p.mode) {
        case Mode::Marathon: texteNiveau = std::to_string(jeu.Niveau()); break;
        case Mode::Sprint:   texteNiveau = FormaterTemps(jeu.Temps()); break;
        case Mode::Ultra:
            texteNiveau = FormaterTemps(jeu.TempsRestant());
            if (jeu.TempsRestant() < 10.f) couleurNiveau = sf::Color(255, 90, 70); // dernières secondes
            break;
        case Mode::Zen:      texteNiveau = FormaterNombre(jeu.Stats().pieces); break;
    }
    DessinerTexte(cible, niveau, Utf8(texteNiveau), 20, Vers(cst::TEXTE_NIVEAU), echelle, 160.f, couleurNiveau);

    // Nom du mode sous la grille
    std::string pied;
    switch (p.mode) {
        case Mode::Marathon:
            pied = "MARATHON";
            if (p.niveauDepart > 0) pied += std::format("{}{}", Tr(" · NIVEAU DE DÉPART ", " · START LEVEL "), p.niveauDepart);
            break;
        case Mode::Sprint: pied = std::format("SPRINT {}{}", p.sprintLignes, Tr(" LIGNES", " LINES")); break;
        case Mode::Ultra:  pied = Tr("ULTRA · 2 MINUTES", "ULTRA · 2 MINUTES"); break;
        case Mode::Zen:    pied = "ZEN"; break;
    }
    DessinerTexte(cible, piedMode, Utf8(pied), 13,
                  {cst::PLATEAU.x + static_cast<float>(cst::TUILE * cst::LARGEUR) / 2.f,
                   cst::PLATEAU.y + static_cast<float>(cst::TUILE * cst::HAUTEUR) + 20.f},
                  echelle, 300.f, sf::Color(150, 150, 150));
}

void Rendu::DessinerCommandes(sf::RenderTarget& cible, const Reglages& reglages, float echelle) {
    cible.draw(masqueCommandes);

    if (reglages.touches != touchesAffichees || echelle != echelleCommandes || LangueActuelle() != langueCommandes) {
        touchesAffichees = reglages.touches;
        echelleCommandes = echelle;
        langueCommandes = LangueActuelle();

        auto principale = [&](Action action) { return touches::Nom(reglages.Touches(action)[0]); };
        const std::array<sf::String, 5> lignesTexte = {
            TrU("Déplacer : ", "Move: ") + principale(Action::Gauche) + " " + principale(Action::Droite),
            TrU("Descendre : ", "Soft drop: ") + principale(Action::DescenteDouce) + TrU("   Chute : ", "   Drop: ") +
                principale(Action::ChuteRapide),
            TrU("Tourner : ", "Rotate: ") + principale(Action::TournerHoraire) + " / " + principale(Action::TournerAntiHoraire),
            TrU("Garder : ", "Hold: ") + principale(Action::Garder),
            TrU("Pause : ", "Pause: ") + principale(Action::Pause) + TrU("   Abandon : ", "   Quit: ") +
                principale(Action::Abandonner),
        };

        const float hauteurLigne = ZONE_COMMANDES.size.y / static_cast<float>(texteCommandes.size());
        for (size_t i = 0; i < texteCommandes.size(); i++) {
            texteCommandes[i].setString(lignesTexte[i]);
            const sf::Vector2f centre(ZONE_COMMANDES.position.x + ZONE_COMMANDES.size.x / 2.f,
                                      ZONE_COMMANDES.position.y + hauteurLigne * (static_cast<float>(i) + 0.5f));
            PlacerTexteBorne(texteCommandes[i], 14, centre, echelle, ZONE_COMMANDES.size.x - 8.f);
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
    motifs.clear();
    motifsActifs = reglages.motifs;
    const sf::Texture* texture = reglages.daltonien ? &tuilesDaltonien : &tuiles;

    const Jeu::Grille& grille = jeu.Plateau();
    for (size_t y = 0; y < grille.size(); y++)
        for (size_t x = 0; x < grille[y].size(); x++)
            if (grille[y][x] != 0) AjouterTuile(grille[y][x], PositionCase(static_cast<int>(x), static_cast<int>(y)));

    if (!jeu.Fini()) {
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
            const auto alpha = static_cast<std::uint8_t>(90.f + (reglages.effets ? 25.f * std::sin(temps * 5.f) : 0.f));
            for (const Case& c : jeu.CasesFantome())
                AjouterTuile(couleur, PositionCase(c.x, c.y) + sf::Vector2f(decalage.x, 0.f), sf::Color(255, 255, 255, alpha), false);
        }

        // La pièce s'assombrit pendant le délai de verrouillage
        const auto luminosite = static_cast<std::uint8_t>(255.f - 100.f * jeu.ProgressionVerrouillage());
        const sf::Color teinte(luminosite, luminosite, luminosite);
        for (const Case& c : jeu.CasesPiece()) AjouterTuile(couleur, PositionCase(c.x, c.y) + decalage, teinte);
    }

    plateau.texture = texture;
    cible.draw(sommets, plateau);
    plateau.texture = nullptr;
    if (motifs.getVertexCount() > 0) cible.draw(motifs, plateau);
    effets.DessinerPlateau(cible, plateau);

    sommets.clear();
    motifs.clear();
    AjouterApercu(jeu.PieceSuivante(), cst::APERCU_SUIVANT, sf::Color::White);
    AjouterApercu(jeu.PieceGardee(), cst::APERCU_GARDE,
                  jeu.GardeUtilisee() ? sf::Color(120, 120, 120) : sf::Color::White);
    cible.draw(sommets, texture);
    if (motifs.getVertexCount() > 0) cible.draw(motifs);

    // Le score affiché rattrape le vrai score en quelques images
    const auto scoreReel = static_cast<double>(jeu.Score());
    if (!reglages.effets || scoreReel < scoreAffiche)
        scoreAffiche = scoreReel;
    else
        scoreAffiche += (scoreReel - scoreAffiche) * static_cast<double>(std::min(1.f, dt * 12.f));
    const long long scoreArrondi = scoreReel - scoreAffiche < 1.0 ? jeu.Score() : static_cast<long long>(scoreAffiche);

    DessinerInfos(cible, jeu, scoreArrondi, echelle);
    DessinerCommandes(cible, reglages, echelle);

    DessinerCombo(cible, jeu, temps, dt, echelle);
    effets.DessinerTextes(cible, echelle);
}

void Rendu::DessinerLimite(sf::RenderTarget& cible, const Jeu& jeu, float temps, const sf::RenderStates& etats) {
    // La ligne limite s'intensifie et clignote quand la pile s'en approche (4 lignes ou moins)
    const auto occupee = [](const auto& ligne) { return std::ranges::any_of(ligne, [](int v) { return v != 0; }); };
    const auto& plateau = jeu.Plateau();
    const int plusHaute = static_cast<int>(std::ranges::find_if(plateau, occupee) - plateau.begin()); // HAUTEUR si vide

    const float danger = std::clamp(static_cast<float>(cst::LIGNES_ZONE_LIMITE + 4 - plusHaute) / 4.f, 0.f, 1.f);
    const float pulsation = 0.5f + 0.5f * std::sin(temps * 9.f);
    limite.setFillColor(sf::Color(255, static_cast<std::uint8_t>(40.f * (1.f - danger)), 0,
                                  static_cast<std::uint8_t>(150.f + 105.f * danger * pulsation)));
    const float epaisseur = 2.f + 2.f * danger;
    if (limite.getSize().y != epaisseur) {
        limite.setSize({static_cast<float>(cst::TUILE * cst::LARGEUR), epaisseur});
        limite.setOrigin({0.f, (epaisseur - 2.f) / 2.f});
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
    couleur.a = static_cast<std::uint8_t>(static_cast<float>(couleur.a) * std::clamp(facteur, 0.f, 1.f));
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
    transformation.translate(centre).scale({zoom, zoom});

    formes.clear();
    const sf::FloatRect badge({-largeur / 2.f, -hauteur / 2.f}, {largeur, hauteur});
    auto agrandi = [&](float marge) {
        return sf::FloatRect(badge.position - sf::Vector2f(marge, marge), badge.size + sf::Vector2f(2.f * marge, 2.f * marge));
    };

    AjouterRectangleArrondi(transformation, agrandi(6.f), rayon + 6.f, Attenuer(sf::Color(couleur.r, couleur.g, couleur.b, 40), fondu * alerte));
    AjouterRectangleArrondi(transformation, agrandi(2.f), rayon + 2.f, Attenuer(couleur, fondu * alerte));
    AjouterRectangleArrondi(transformation, badge, rayon, Attenuer(sf::Color(18, 18, 18, 235), fondu));
    // Temps restant : remplissage qui se vide de droite à gauche
    AjouterRectangleArrondi(transformation, {badge.position, {badge.size.x * ratio, badge.size.y}}, rayon,
                            Attenuer(sf::Color(couleur.r, couleur.g, couleur.b, 70), fondu));
    // Éclair blanc à chaque nouveau palier
    AjouterRectangleArrondi(transformation, badge, rayon,
                            Attenuer(sf::Color(255, 255, 255, 200), std::exp(-14.f * tempsCombo) * fondu));
    cible.draw(formes);

    const auto alpha = static_cast<std::uint8_t>(255.f * fondu);
    texteComboLabel.setFillColor(sf::Color(220, 220, 220, alpha));
    PlacerTexte(texteComboLabel, 11, transformation.transformPoint({-30.f, 0.f}), echelle, zoom);
    cible.draw(texteComboLabel);

    texteCombo.setFillColor(sf::Color(couleur.r, couleur.g, couleur.b, alpha));
    PlacerTexte(texteCombo, 18, transformation.transformPoint({40.f, 0.f}), echelle, zoom);
    cible.draw(texteCombo);
}
