#include "Rendu.h"

#include "Formes.h"
#include "Palette.h"
#include "Texte.h"
#include "Theme.h"
#include "Touches.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr float T = static_cast<float>(cst::TUILE);
constexpr sf::Vector2f ORIGINE(cst::PLATEAU.x, cst::PLATEAU.y);
constexpr sf::FloatRect GRILLE(ORIGINE, {T * cst::LARGEUR, T * cst::HAUTEUR});

// Colonnes de part et d'autre du plateau (zone logique de 900 × 540)
constexpr sf::FloatRect CADRE_RESERVE({150.f, 50.f}, {170.f, 104.f});
constexpr sf::FloatRect CADRE_STATS({150.f, 170.f}, {170.f, 236.f});
constexpr sf::FloatRect CADRE_SUIVANTES({580.f, 50.f}, {170.f, 292.f});
constexpr sf::FloatRect CADRE_COMMANDES({580.f, 358.f}, {170.f, 132.f});
constexpr sf::Vector2f BADGE_COMBO(235.f, 448.f);
constexpr float RAYON = 12.f; // coins des encadrés

constexpr char32_t SIGNE_FOIS = U'×';

sf::Vector2f PositionCase(int x, int y) {
    return ORIGINE + sf::Vector2f(T * static_cast<float>(x), T * static_cast<float>(y));
}

sf::FloatRect Agrandi(sf::FloatRect zone, float marge) {
    return {zone.position - sf::Vector2f(marge, marge), zone.size + sf::Vector2f(2.f * marge, 2.f * marge)};
}

sf::Color Attenuer(sf::Color couleur, float facteur) {
    couleur.a = static_cast<std::uint8_t>(static_cast<float>(couleur.a) * std::clamp(facteur, 0.f, 1.f));
    return couleur;
}

// Rectangle plein d'une seule couleur
void AjouterRectangle(sf::VertexArray& sommets, sf::FloatRect zone, sf::Color couleur) {
    const sf::Vector2f a = zone.position, c = zone.position + zone.size;
    formes::AjouterQuad(sommets, {a, couleur}, {{c.x, a.y}, couleur}, {c, couleur}, {{a.x, c.y}, couleur});
}

// Encadré : fond sombre et fin liseré clair
void AjouterEncadre(sf::VertexArray& sommets, sf::FloatRect zone) {
    formes::AjouterRectangleArrondi(sommets, zone, RAYON, theme::PANNEAU);
    formes::AjouterContourArrondi(sommets, zone, RAYON, 1.f, theme::BORD);
}

// 0 quand la pile est loin du haut, 1 quand elle atteint la ligne limite
float Danger(const Jeu& jeu) {
    const auto occupee = [](const auto& ligne) { return std::ranges::any_of(ligne, [](int v) { return v != 0; }); };
    const auto& grille = jeu.Plateau();
    const int plusHaute = static_cast<int>(std::ranges::find_if(grille, occupee) - grille.begin()); // HAUTEUR si vide
    return std::clamp(static_cast<float>(cst::LIGNES_ZONE_LIMITE + 4 - plusHaute) / 4.f, 0.f, 1.f);
}

// Motifs d'accessibilité, en coordonnées locales d'une case de 18 (mis à l'échelle au dessin)
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

// Couleur du badge selon le palier (couleurs des pièces, pour rester cohérent avec le jeu)
sf::Color CouleurCombo(int combo) {
    if (combo >= 8) return {230, 80, 200};
    if (combo >= 5) return {235, 125, 36};
    if (combo >= 3) return {244, 200, 36};
    if (combo >= 2) return {102, 191, 41};
    return {48, 190, 229};
}

} // namespace

Rendu::Rendu(const sf::Font& policeTexte) : police(policeTexte) {
    // Capacité réservée une fois : plus de réallocation pendant la partie (30 sommets par bloc)
    decor.resize(1500);
    decor.clear();
    plateau.resize(30 * (cst::LARGEUR * cst::HAUTEUR + 16) + 1500);
    plateau.clear();
    motifs.resize(6 * 4 * (cst::LARGEUR * cst::HAUTEUR + 16));
    motifs.clear();
    formes.resize(30 * 4 * (NB_SUIVANTES + 1) + 6 * 24 * 5);
    formes.clear();

    for (TexteCache& c : etiquettes) c.texte.setLetterSpacing(2.2f);
    piedMode.texte.setLetterSpacing(2.2f);
    for (auto* groupe : {&actionsCommandes, &touchesCommandes})
        for (TexteCache& c : *groupe) c.texte.setStyle(sf::Text::Bold);
    for (TexteCache& c : etiquettes) c.texte.setStyle(sf::Text::Bold);
    for (TexteCache& c : valeurs) c.texte.setStyle(sf::Text::Bold);
    piedMode.texte.setStyle(sf::Text::Bold);
    texteCombo.setStyle(sf::Text::Bold);
    texteComboLabel.setStyle(sf::Text::Bold);
    texteComboLabel.setLetterSpacing(2.f);
}

void Rendu::PrechargerGlyphes(float echelle) {
    echellePrechargee = echelle;
    // Avec « × » du combo et l'espace insécable des nombres
    static const std::u32string caracteres = U"0123456789+ !,.:/-ABCDEFGHIJKLMNOPQRSTUVWXYZ×\xA0";

    const float contour = 2.f * echelle;
    // Tailles logiques des textes de la partie : valeurs, combo, textes flottants
    struct Style {
        unsigned taille;
        bool avecContour;
    };
    constexpr std::array<Style, 9> styles = {
        {{22, false}, {26, false}, {11, false}, {18, false}, {18, true}, {22, true}, {24, true}, {28, true}, {30, true}}};

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

void Rendu::AjouterBloc(sf::VertexArray& sommets, int couleur, sf::FloatRect zone, sf::Color teinte, bool daltonien,
                        bool avecMotif) {
    couleur = std::clamp(couleur, 1, 7);
    formes::AjouterMino(sommets, zone, palette::Tuile(couleur, daltonien), teinte);

    if (!motifsActifs || !avecMotif) return;
    const float echelle = zone.size.x / 18.f;
    const sf::Color encre(0, 0, 0, static_cast<std::uint8_t>(teinte.a * 120 / 255));
    const auto point = [&](sf::Vector2f p) { return sf::Vertex{zone.position + p * echelle, encre}; };
    for (const Quad& q : Motif(couleur)) formes::AjouterQuad(motifs, point(q[0]), point(q[1]), point(q[2]), point(q[3]));
}

void Rendu::AjouterApercu(std::optional<TypePiece> type, sf::Vector2f centre, float cote, sf::Color teinte,
                          bool daltonien) {
    if (!type) return;

    // Centre la pièce dans son emplacement d'après ses cases réelles
    const Cases cases = piece::Forme(*type, 0);
    const auto [minX, maxX] = std::ranges::minmax(cases | std::views::transform(&Case::x));
    const auto [minY, maxY] = std::ranges::minmax(cases | std::views::transform(&Case::y));
    const sf::Vector2f taille(static_cast<float>(maxX - minX + 1), static_cast<float>(maxY - minY + 1));
    const sf::Vector2f origine = centre - taille * cote / 2.f;

    for (const Case& c : cases) {
        const sf::Vector2f position(static_cast<float>(c.x - minX), static_cast<float>(c.y - minY));
        AjouterBloc(formes, piece::Couleur(*type), {origine + position * cote, {cote, cote}}, teinte, daltonien);
    }
}

void Rendu::DessinerTexte(sf::RenderTarget& cible, TexteCache& cache, const sf::String& chaine, unsigned taille,
                          sf::Vector2f point, float echelle, float largeurMax, sf::Color couleur, Alignement alignement) {
    if (chaine != cache.chaine || echelle != cache.echelle) {
        cache.chaine = chaine;
        cache.echelle = echelle;
        cache.texte.setString(chaine);
        PlacerTexteBorne(cache.texte, taille, point, echelle, largeurMax);
        if (alignement != Alignement::Centre) {
            const float demiLargeur = cache.texte.getGlobalBounds().size.x / 2.f;
            cache.texte.move({alignement == Alignement::Gauche ? demiLargeur : -demiLargeur, 0.f});
        }
    }
    cache.texte.setFillColor(couleur);
    cible.draw(cache.texte);
}

void Rendu::DessinerDecor(sf::RenderTarget& cible) {
    decor.clear();
    // Dégradé sur toute la zone visible : plus large ou plus haute que 900 × 540 selon la fenêtre
    const sf::View& vue = cible.getView();
    const sf::Vector2f coin = vue.getCenter() - vue.getSize() / 2.f, taille = vue.getSize();
    formes::AjouterQuad(decor, {coin, theme::FOND_HAUT}, {coin + sf::Vector2f(taille.x, 0.f), theme::FOND_HAUT},
                        {coin + taille, theme::FOND_BAS}, {coin + sf::Vector2f(0.f, taille.y), theme::FOND_BAS});
    for (const sf::FloatRect& cadre : {CADRE_RESERVE, CADRE_STATS, CADRE_SUIVANTES, CADRE_COMMANDES})
        AjouterEncadre(decor, cadre);
    cible.draw(decor);
}

void Rendu::DessinerPlateau(sf::RenderTarget& cible, const Jeu& jeu, float temps, float dt, const Reglages& reglages,
                            Effets& effets) {
    // Le plateau et ses effets tremblent ensemble ; le reste de l'interface reste fixe
    sf::RenderStates etats;
    etats.transform.translate(effets.Secousse());
    plateau.clear();
    motifs.clear();
    motifsActifs = reglages.motifs;
    const bool daltonien = reglages.daltonien;

    // Le cadre rougit et palpite quand la pile approche de la ligne limite
    const float danger = Danger(jeu);
    const float pulsation = 0.5f + 0.5f * std::sin(temps * 9.f);
    const sf::Color cadre = palette::Melanger(theme::CADRE, {255, 70, 60, 230}, danger * (0.6f + 0.4f * pulsation));
    for (int i = 1; i <= 3; i++) {
        const float marge = 4.f + 3.f * static_cast<float>(i);
        formes::AjouterContourArrondi(plateau, Agrandi(GRILLE, marge), 4.f + marge, 3.f,
                                      Attenuer(cadre, 0.24f / static_cast<float>(i)));
    }
    formes::AjouterRectangleArrondi(plateau, Agrandi(GRILLE, 4.f), 8.f, theme::PLATEAU);
    formes::AjouterContourArrondi(plateau, Agrandi(GRILLE, 4.f), 8.f, 1.5f, cadre);

    // Zone au-dessus de la ligne limite, voilée de rouge d'autant plus que la pile monte
    const float hauteurZone = T * static_cast<float>(cst::LIGNES_ZONE_LIMITE);
    AjouterRectangle(plateau, {ORIGINE, {GRILLE.size.x, hauteurZone}},
                     {255, 60, 60, static_cast<std::uint8_t>(8.f + 22.f * danger)});

    // Quadrillage discret
    for (int x = 1; x < cst::LARGEUR; x++)
        AjouterRectangle(plateau, {PositionCase(x, 0) - sf::Vector2f(0.5f, 0.f), {1.f, GRILLE.size.y}}, theme::GRILLE);
    for (int y = 1; y < cst::HAUTEUR; y++)
        AjouterRectangle(plateau, {PositionCase(0, y) - sf::Vector2f(0.f, 0.5f), {GRILLE.size.x, 1.f}}, theme::GRILLE);

    // Ligne limite : s'épaissit et palpite quand la pile s'en approche
    const float epaisseur = 2.f + 2.f * danger;
    AjouterRectangle(plateau, {{ORIGINE.x, ORIGINE.y + hauteurZone - epaisseur / 2.f}, {GRILLE.size.x, epaisseur}},
                     {255, static_cast<std::uint8_t>(40.f * (1.f - danger)), 0,
                      static_cast<std::uint8_t>(150.f + 105.f * danger * pulsation)});

    // Blocs posés
    const Jeu::Grille& grille = jeu.Plateau();
    for (size_t y = 0; y < grille.size(); y++)
        for (size_t x = 0; x < grille[y].size(); x++)
            if (grille[y][x] != 0)
                AjouterBloc(plateau, grille[y][x], {PositionCase(static_cast<int>(x), static_cast<int>(y)), {T, T}},
                            sf::Color::White, daltonien);

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
        const sf::Vector2f decalage = (positionAffichee - position) * T;

        const int couleur = piece::Couleur(jeu.PieceActive());
        if (reglages.fantome) {
            // Fantôme qui respire légèrement ; il suit la pièce horizontalement
            const auto alpha = static_cast<std::uint8_t>(90.f + (reglages.effets ? 25.f * std::sin(temps * 5.f) : 0.f));
            for (const Case& c : jeu.CasesFantome())
                AjouterBloc(plateau, couleur, {PositionCase(c.x, c.y) + sf::Vector2f(decalage.x, 0.f), {T, T}},
                            {255, 255, 255, alpha}, daltonien, false);
        }

        // La pièce s'assombrit pendant le délai de verrouillage
        const auto luminosite = static_cast<std::uint8_t>(255.f - 100.f * jeu.ProgressionVerrouillage());
        for (const Case& c : jeu.CasesPiece())
            AjouterBloc(plateau, couleur, {PositionCase(c.x, c.y) + decalage, {T, T}},
                        {luminosite, luminosite, luminosite}, daltonien);
    }

    cible.draw(plateau, etats);
    if (motifs.getVertexCount() > 0) cible.draw(motifs, etats);
    effets.DessinerPlateau(cible, etats);
}

void Rendu::DessinerApercus(sf::RenderTarget& cible, const Jeu& jeu, const Reglages& reglages) {
    formes.clear();
    motifs.clear();
    const bool daltonien = reglages.daltonien;

    // Pièce en réserve, grisée tant qu'elle ne peut pas être reprise
    AjouterApercu(jeu.PieceGardee(), {CADRE_RESERVE.getCenter().x, CADRE_RESERVE.position.y + 62.f}, 18.f,
                  jeu.GardeUtilisee() ? sf::Color(120, 120, 120) : sf::Color::White, daltonien);

    // File des suivantes : la prochaine en grand, les autres plus petites
    const auto suivantes = jeu.Suivantes();
    const float x = CADRE_SUIVANTES.getCenter().x;
    AjouterApercu(suivantes.front(), {x, CADRE_SUIVANTES.position.y + 62.f}, 18.f, sf::Color::White, daltonien);
    for (size_t i = 1; i < suivantes.size(); i++)
        AjouterApercu(suivantes[i], {x, CADRE_SUIVANTES.position.y + 115.f + 44.f * static_cast<float>(i - 1)}, 14.f,
                      sf::Color::White, daltonien);

    cible.draw(formes);
    if (motifs.getVertexCount() > 0) cible.draw(motifs);
}

void Rendu::DessinerInfos(sf::RenderTarget& cible, const Jeu& jeu, long long scoreAffichage, float echelle) {
    const ParametresPartie& p = jeu.Parametres();
    const float gauche = CADRE_STATS.getCenter().x, haut = CADRE_STATS.position.y;
    const float largeurMax = CADRE_STATS.size.x - 20.f;

    DessinerTexte(cible, etiquettes[0], TrU("RÉSERVE", "HOLD"), 12, {gauche, CADRE_RESERVE.position.y + 20.f}, echelle,
                  largeurMax, theme::ETIQUETTE);
    DessinerTexte(cible, etiquettes[1], TrU("SUIVANTES", "NEXT"), 12,
                  {CADRE_SUIVANTES.getCenter().x, CADRE_SUIVANTES.position.y + 20.f}, echelle, largeurMax, theme::ETIQUETTE);

    // Score, doré quand le record est battu
    const bool recordBattu = record > 0 && jeu.Score() > record;
    DessinerTexte(cible, etiquettes[2], Utf8("SCORE"), 12, {gauche, haut + 24.f}, echelle, largeurMax, theme::ETIQUETTE);
    DessinerTexte(cible, valeurs[0], Utf8(FormaterNombre(scoreAffichage)), 26, {gauche, haut + 52.f}, echelle,
                  largeurMax, recordBattu ? theme::OR : theme::TEXTE);

    const std::string texteLignes = p.mode == Mode::Sprint ? std::format("{} / {}", jeu.Lignes(), p.sprintLignes)
                                                           : FormaterNombre(jeu.Lignes());
    DessinerTexte(cible, etiquettes[3], TrU("LIGNES", "LINES"), 12, {gauche, haut + 94.f}, echelle, largeurMax,
                  theme::ETIQUETTE);
    DessinerTexte(cible, valeurs[1], Utf8(texteLignes), 22, {gauche, haut + 120.f}, echelle, largeurMax);

    // Troisième valeur selon le mode, avec une barre de progression (niveau suivant, objectif, temps restant)
    sf::String etiquette;
    std::string valeur;
    sf::Color couleur = theme::TEXTE;
    float progression = -1.f;
    switch (p.mode) {
        case Mode::Marathon:
            etiquette = TrU("NIVEAU", "LEVEL");
            valeur = std::to_string(jeu.Niveau());
            progression = jeu.Niveau() >= cst::NIVEAU_MAX
                              ? 1.f
                              : static_cast<float>(jeu.Lignes() % cst::LIGNES_PAR_NIVEAU) / cst::LIGNES_PAR_NIVEAU;
            break;
        case Mode::Sprint:
            etiquette = TrU("TEMPS", "TIME");
            valeur = FormaterTemps(jeu.Temps());
            progression = static_cast<float>(jeu.Lignes()) / static_cast<float>(std::max(1, p.sprintLignes));
            break;
        case Mode::Ultra:
            etiquette = TrU("RESTANT", "TIME LEFT");
            valeur = FormaterTemps(jeu.TempsRestant());
            if (jeu.TempsRestant() < 10.f) couleur = theme::ALERTE; // dernières secondes
            progression = jeu.TempsRestant() / mode::ULTRA_DUREE_S;
            break;
        case Mode::Zen:
            etiquette = TrU("PIÈCES", "PIECES");
            valeur = FormaterNombre(jeu.Stats().pieces);
            break;
    }
    DessinerTexte(cible, etiquettes[4], etiquette, 12, {gauche, haut + 162.f}, echelle, largeurMax, theme::ETIQUETTE);
    DessinerTexte(cible, valeurs[2], Utf8(valeur), 22, {gauche, haut + 188.f}, echelle, largeurMax, couleur);

    if (progression >= 0.f) {
        formes.clear();
        const sf::FloatRect barre({gauche - 60.f, haut + 212.f}, {120.f, 5.f});
        formes::AjouterRectangleArrondi(formes, barre, 2.5f, {255, 255, 255, 28});
        formes::AjouterRectangleArrondi(formes, {barre.position, {barre.size.x * std::clamp(progression, 0.f, 1.f), barre.size.y}},
                                        2.5f, couleur == theme::ALERTE ? theme::ALERTE : CouleurCombo(1));
        cible.draw(formes);
    }

    // Nom du mode au-dessus du plateau
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
    DessinerTexte(cible, piedMode, Utf8(pied), 12, {GRILLE.getCenter().x, GRILLE.position.y - 24.f}, echelle, 400.f,
                  theme::ETIQUETTE);
}

void Rendu::DessinerCommandes(sf::RenderTarget& cible, const Reglages& reglages, float echelle) {
    const sf::FloatRect& cadre = CADRE_COMMANDES;
    DessinerTexte(cible, etiquettes[5], TrU("COMMANDES", "CONTROLS"), 12, {cadre.getCenter().x, cadre.position.y + 20.f},
                  echelle, cadre.size.x - 20.f, theme::ETIQUETTE);

    // Action à gauche, touche principale à droite : la configuration réelle du joueur
    const auto touche = [&](Action action) { return touches::Nom(reglages.Touches(action)[0]); };
    const std::array<std::pair<sf::String, sf::String>, 6> lignes = {{
        {TrU("Déplacer", "Move"), touche(Action::Gauche) + " " + touche(Action::Droite)},
        {TrU("Descente", "Soft drop"), touche(Action::DescenteDouce)},
        {TrU("Chute", "Hard drop"), touche(Action::ChuteRapide)},
        {TrU("Tourner", "Rotate"), touche(Action::TournerHoraire) + " / " + touche(Action::TournerAntiHoraire)},
        {TrU("Réserve", "Hold"), touche(Action::Garder)},
        {TrU("Pause", "Pause"), touche(Action::Pause)},
    }};
    for (size_t i = 0; i < lignes.size(); i++) {
        const float y = cadre.position.y + 42.f + 16.f * static_cast<float>(i);
        DessinerTexte(cible, actionsCommandes[i], lignes[i].first, 12, {cadre.position.x + 14.f, y}, echelle, 70.f,
                      theme::DISCRET, Alignement::Gauche);
        DessinerTexte(cible, touchesCommandes[i], lignes[i].second, 12, {cadre.position.x + cadre.size.x - 14.f, y},
                      echelle, 80.f, theme::TEXTE, Alignement::Droite);
    }
}

void Rendu::Dessiner(sf::RenderTarget& cible, const Jeu& jeu, float temps, float echelle, const Reglages& reglages,
                     Effets& effets) {
    const float dt = std::clamp(temps - dernierTemps, 0.f, 0.1f);
    dernierTemps = temps;
    if (echelle != echellePrechargee) PrechargerGlyphes(echelle);

    DessinerDecor(cible);
    DessinerPlateau(cible, jeu, temps, dt, reglages, effets);
    DessinerApercus(cible, jeu, reglages);

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
    const float largeur = 150.f, hauteur = 30.f, rayon = hauteur / 2.f;

    sf::Transform transformation;
    transformation.translate(BADGE_COMBO).scale({zoom, zoom});

    formes.clear();
    const sf::FloatRect badge({-largeur / 2.f, -hauteur / 2.f}, {largeur, hauteur});
    formes::AjouterRectangleArrondi(formes, Agrandi(badge, 6.f), rayon + 6.f,
                                    Attenuer(sf::Color(couleur.r, couleur.g, couleur.b, 40), fondu * alerte),
                                    transformation);
    formes::AjouterRectangleArrondi(formes, Agrandi(badge, 2.f), rayon + 2.f, Attenuer(couleur, fondu * alerte),
                                    transformation);
    formes::AjouterRectangleArrondi(formes, badge, rayon, Attenuer(sf::Color(18, 18, 18, 235), fondu), transformation);
    // Temps restant : remplissage qui se vide de droite à gauche
    formes::AjouterRectangleArrondi(formes, {badge.position, {badge.size.x * ratio, badge.size.y}}, rayon,
                                    Attenuer(sf::Color(couleur.r, couleur.g, couleur.b, 70), fondu), transformation);
    // Éclair blanc à chaque nouveau palier
    formes::AjouterRectangleArrondi(formes, badge, rayon,
                                    Attenuer(sf::Color(255, 255, 255, 200), std::exp(-14.f * tempsCombo) * fondu),
                                    transformation);
    cible.draw(formes);

    const auto alpha = static_cast<std::uint8_t>(255.f * fondu);
    texteComboLabel.setFillColor(sf::Color(220, 220, 220, alpha));
    PlacerTexte(texteComboLabel, 11, transformation.transformPoint({-30.f, 0.f}), echelle, zoom);
    cible.draw(texteComboLabel);

    texteCombo.setFillColor(sf::Color(couleur.r, couleur.g, couleur.b, alpha));
    PlacerTexte(texteCombo, 18, transformation.transformPoint({40.f, 0.f}), echelle, zoom);
    cible.draw(texteCombo);
}
