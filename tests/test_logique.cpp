// Tests de la logique du jeu, sans fenêtre ni SFML
#include "Jeu.h"
#include "MeilleurScore.h"
#include "Piece.h"
#include "Reglages.h"
#include "Repetition.h"
#include "Sac.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>

namespace {

int echecs = 0;

#define VERIFIER(condition)                                                          \
    do {                                                                             \
        if (!(condition)) {                                                          \
            std::cerr << __FILE__ << ':' << __LINE__ << " : " #condition " est faux\n"; \
            ++echecs;                                                                \
        }                                                                            \
    } while (0)

std::set<std::pair<int, int>> Ensemble(const Cases& cases) {
    std::set<std::pair<int, int>> resultat;
    for (const Case& c : cases) resultat.insert({c.x, c.y});
    return resultat;
}

int MinX(const Cases& cases) {
    return std::min_element(cases.begin(), cases.end(), [](Case a, Case b) { return a.x < b.x; })->x;
}

int MinY(const Cases& cases) {
    return std::min_element(cases.begin(), cases.end(), [](Case a, Case b) { return a.y < b.y; })->y;
}

// Trouve une graine qui donne `type` comme première pièce
Jeu AvecPremiere(TypePiece type, const Jeu::Grille& grille = {}) {
    for (unsigned graine = 0;; graine++) {
        Jeu jeu(graine, grille);
        if (jeu.PieceActive() == type) return jeu;
    }
}

void TestSac() {
    Sac sac(42);
    for (int serie = 0; serie < 3; serie++) {
        std::set<TypePiece> vues;
        for (int i = 0; i < NB_TYPES; i++) vues.insert(sac.Tirer());
        VERIFIER(static_cast<int>(vues.size()) == NB_TYPES);
    }
}

void TestFormes() {
    for (int t = 0; t < NB_TYPES; t++) {
        const TypePiece type = static_cast<TypePiece>(t);
        VERIFIER(Ensemble(piece::Forme(type, 0)) == Ensemble(piece::Forme(type, 4)));
        VERIFIER(Ensemble(piece::Forme(type, -1)) == Ensemble(piece::Forme(type, 3)));
    }
    // États SRS de référence
    VERIFIER(Ensemble(piece::Forme(TypePiece::T, 1)) == Ensemble({{{1, 0}, {1, 1}, {2, 1}, {1, 2}}}));
    VERIFIER(Ensemble(piece::Forme(TypePiece::I, 1)) == Ensemble({{{2, 0}, {2, 1}, {2, 2}, {2, 3}}}));
    VERIFIER(Ensemble(piece::Forme(TypePiece::I, 2)) == Ensemble({{{0, 2}, {1, 2}, {2, 2}, {3, 2}}}));
    VERIFIER(Ensemble(piece::Forme(TypePiece::O, 1)) == Ensemble(piece::Forme(TypePiece::O, 0)));
}

void TestDeplacementsBornes() {
    Jeu jeu = AvecPremiere(TypePiece::T);
    for (int i = 0; i < 20; i++) jeu.Deplacer(-1);
    VERIFIER(MinX(jeu.CasesPiece()) == 0);
    VERIFIER(!jeu.Deplacer(-1));
}

void TestWallKick() {
    Jeu jeu = AvecPremiere(TypePiece::T);
    VERIFIER(jeu.Tourner(true)); // état R : la pièce occupe les colonnes 4 et 5
    while (jeu.Deplacer(-1)) {}
    VERIFIER(MinX(jeu.CasesPiece()) == 0);
    // R -> 2 sans décalage sortirait du plateau : le test SRS (+1, 0) doit réussir
    VERIFIER(jeu.Tourner(true));
    VERIFIER(MinX(jeu.CasesPiece()) == 0);
}

void TestLigneEtScore() {
    Jeu::Grille grille{};
    for (int x = 0; x < cst::LARGEUR; x++)
        if (x < 3 || x > 6) grille[cst::HAUTEUR - 1][x] = 1;

    Jeu jeu = AvecPremiere(TypePiece::I, grille);
    jeu.ChuteRapide();

    VERIFIER(jeu.Lignes() == 1);
    VERIFIER(jeu.Combo() == 1);
    // 18 cases de chute rapide * 2 + 100 pour la ligne + 50 de combo
    VERIFIER(jeu.Score() == 186);
    for (int valeur : jeu.Plateau()[cst::HAUTEUR - 1]) VERIFIER(valeur == 0);
    VERIFIER(!jeu.Perdu());

    // Événements pour les effets : chute, verrouillage puis ligne (avec son contenu d'avant effacement)
    const auto& ev = jeu.Evenements();
    VERIFIER(ev.size() == 3);
    if (ev.size() == 3) {
        VERIFIER(ev[0].type == EvenementJeu::Type::ChuteRapide && ev[0].distance == 18);
        VERIFIER(ev[1].type == EvenementJeu::Type::Verrouillage);
        VERIFIER(ev[2].type == EvenementJeu::Type::Lignes && ev[2].nbLignes == 1 && ev[2].lignes[0] == cst::HAUTEUR - 1);
        VERIFIER(ev[2].points == 150 && ev[2].contenu[0][0] == 1 && ev[2].contenu[0][4] == piece::Couleur(TypePiece::I));
    }
    jeu.ViderEvenements();
    VERIFIER(jeu.Evenements().empty());
}

void TestDefaite() {
    Jeu::Grille grille{};
    for (int y = cst::LIGNES_ZONE_LIMITE; y < cst::HAUTEUR; y++)
        for (int x = 0; x < cst::LARGEUR - 1; x++) grille[y][x] = 1; // pas de ligne complète

    Jeu jeu(1, grille);
    jeu.ChuteRapide();
    VERIFIER(jeu.Perdu());
}

void TestGarde() {
    Jeu jeu(7);
    const TypePiece premiere = jeu.PieceActive();
    const TypePiece suivante = jeu.PieceSuivante();

    // Autorisée même après être descendu
    for (int i = 0; i < 8; i++) jeu.DescenteDouce();
    jeu.Garder();
    VERIFIER(jeu.PieceGardee() == premiere);
    VERIFIER(jeu.PieceActive() == suivante);
    VERIFIER(MinY(jeu.CasesPiece()) <= 1);

    // Une seule fois par pièce
    jeu.Garder();
    VERIFIER(jeu.PieceActive() == suivante);

    // De nouveau possible après verrouillage
    jeu.ChuteRapide();
    VERIFIER(!jeu.GardeUtilisee());
}

void TestGravite() {
    Jeu jeu(3);
    const int y = MinY(jeu.CasesPiece());
    jeu.MettreAJour(jeu.IntervalleGravite() / 2.f);
    VERIFIER(MinY(jeu.CasesPiece()) == y);
    jeu.MettreAJour(jeu.IntervalleGravite());
    VERIFIER(MinY(jeu.CasesPiece()) == y + 1);
    VERIFIER(jeu.IntervalleGravite() < 1.f);
}

void TestRepetition() {
    RepetitionTouche touche(0.2f, 0.05f, 10);
    VERIFIER(touche.MettreAJour(0.1f) == 0); // non enfoncée
    touche.Appuyer();
    VERIFIER(touche.MettreAJour(0.01f) == 1); // appui immédiat
    VERIFIER(touche.MettreAJour(0.1f) == 0);  // pendant le délai initial
    VERIFIER(touche.MettreAJour(0.11f) == 1); // fin du délai
    VERIFIER(touche.MettreAJour(0.1f) == 2);  // deux intervalles
    VERIFIER(touche.MettreAJour(5.f) == 10);  // gros ralentissement : plafonné
    touche.Relacher();
    VERIFIER(touche.MettreAJour(1.f) == 0);

    // Intervalle nul : déplacement instantané jusqu'au maximum après le délai
    RepetitionTouche instantanee(0.1f, 0.f, 10);
    instantanee.Appuyer();
    VERIFIER(instantanee.MettreAJour(0.01f) == 1);
    VERIFIER(instantanee.MettreAJour(0.05f) == 0);
    VERIFIER(instantanee.MettreAJour(0.06f) == 10);
}

int CasesOccupees(const Jeu& jeu) {
    int total = 0;
    for (const auto& ligne : jeu.Plateau())
        for (int valeur : ligne) total += valeur != 0;
    return total;
}

void TestDelaiVerrouillage() {
    Jeu jeu(5);
    jeu.DefinirDelaiVerrouillage(0.5f);
    while (jeu.DescenteDouce()) {}

    jeu.MettreAJour(0.3f);
    VERIFIER(CasesOccupees(jeu) == 0); // pas encore verrouillée
    VERIFIER(jeu.ProgressionVerrouillage() > 0.f);

    // Un mouvement au sol relance le délai
    VERIFIER(jeu.Deplacer(1) || jeu.Deplacer(-1));
    VERIFIER(jeu.ProgressionVerrouillage() == 0.f);
    jeu.MettreAJour(0.3f);
    VERIFIER(CasesOccupees(jeu) == 0);

    jeu.MettreAJour(0.3f);
    VERIFIER(CasesOccupees(jeu) == 4);

    // Les relances sont limitées : la pièce finit par se verrouiller malgré les mouvements
    Jeu infini(6);
    infini.DefinirDelaiVerrouillage(0.5f);
    while (infini.DescenteDouce()) {}
    for (int i = 0; i < 100 && CasesOccupees(infini) == 0; i++) {
        infini.MettreAJour(0.1f);
        if (!infini.Deplacer(1)) infini.Deplacer(-1);
    }
    VERIFIER(CasesOccupees(infini) == 4);
}

// Faux clavier pour tester le fichier de réglages sans SFML
const std::map<std::string, int> CODES = {{"Left", 1}, {"Right", 2}, {"A", 3}, {"P", 4}};

std::optional<int> CodeTest(const std::string& nom) {
    const auto it = CODES.find(nom);
    return it == CODES.end() ? std::nullopt : std::optional<int>(it->second);
}

std::string NomTest(int code) {
    for (const auto& [nom, c] : CODES)
        if (c == code) return nom;
    return "?";
}

void TestReglages() {
    Reglages defauts;
    defauts.AssignerTouche(Action::Gauche, 1);
    defauts.AssignerTouche(Action::Droite, 2);
    defauts.AssignerTouche(Action::Pause, 4);

    const Reglages r = reglages::Analyser("# commentaire\n"
                                          "das_ms = 9999\n"
                                          "arr_ms = abc\n"
                                          "  fantome=non  \n"
                                          "ligne sans egal\n"
                                          "touche.gauche = A, Left\n"
                                          "touche.pause =\n"
                                          "touche.droite = Inconnue\n"
                                          "touche.inexistante = A\n",
                                          defauts, CodeTest);
    VERIFIER(r.dasMs == Reglages::BORNES_DAS.max); // borné
    VERIFIER(r.arrMs == defauts.arrMs);            // invalide : défaut conservé
    VERIFIER(!r.fantome);
    VERIFIER(r.Touches(Action::Gauche)[0] == 3 && r.Touches(Action::Gauche)[1] == 1);
    VERIFIER(r.Touches(Action::Pause)[0] == AUCUNE_TOUCHE); // vide = aucune touche
    VERIFIER(r.Touches(Action::Droite)[0] == 2);            // nom inconnu : défaut conservé
    VERIFIER(r.ActionDe(3) == Action::Gauche);

    // Aller-retour par le fichier
    const Reglages relu = reglages::Analyser(reglages::Serialiser(r, NomTest), Reglages{}, CodeTest);
    VERIFIER(relu.dasMs == r.dasMs && relu.arrMs == r.arrMs && relu.fantome == r.fantome);
    VERIFIER(relu.effets == r.effets && relu.secousses == r.secousses);
    VERIFIER(relu.touches == r.touches);

    // Une touche ne peut servir qu'à une action
    Reglages conflit = r;
    conflit.AssignerTouche(Action::Droite, 1);
    VERIFIER(conflit.Touches(Action::Droite)[0] == 1 && conflit.Touches(Action::Droite)[1] == 2);
    VERIFIER(conflit.Touches(Action::Gauche)[0] == 3 && conflit.Touches(Action::Gauche)[1] == AUCUNE_TOUCHE);
}

void TestMeilleurScore() {
    namespace fs = std::filesystem;
    const fs::path dossier = fs::temp_directory_path() / "tetris_tests_score";
    const fs::path fichier = dossier / "sous" / "score.txt";
    fs::remove_all(dossier);

    VERIFIER(meilleur_score::Charger(fichier) == 0);
    VERIFIER(meilleur_score::Sauvegarder(fichier, 12345));
    VERIFIER(meilleur_score::Charger(fichier) == 12345);
    VERIFIER(meilleur_score::Sauvegarder(fichier, 99));
    VERIFIER(meilleur_score::Charger(fichier) == 99);

    for (const char* invalide : {"abc", "-5", "12abc", "", "99999999999999999999999"}) {
        std::ofstream(fichier) << invalide;
        VERIFIER(meilleur_score::Charger(fichier) == 0);
    }
    // Fichier démesuré ou qui n'est pas un fichier ordinaire : ignoré
    std::ofstream(fichier) << std::string(200 * 1024, '7');
    VERIFIER(meilleur_score::Charger(fichier) == 0);
    VERIFIER(meilleur_score::Charger(dossier) == 0);

    // Un temporaire oublié par une écriture interrompue n'empêche pas de sauvegarder
    std::ofstream(fs::path(fichier) += ".tmp") << "reste";
    VERIFIER(meilleur_score::Sauvegarder(fichier, 42));
    VERIFIER(meilleur_score::Charger(fichier) == 42);
    VERIFIER(!fs::exists(fs::path(fichier) += ".tmp"));
    fs::remove_all(dossier);
}

} // namespace

int main() {
    TestSac();
    TestFormes();
    TestDeplacementsBornes();
    TestWallKick();
    TestLigneEtScore();
    TestDefaite();
    TestGarde();
    TestGravite();
    TestRepetition();
    TestDelaiVerrouillage();
    TestReglages();
    TestMeilleurScore();

    if (echecs == 0) std::cout << "Tous les tests passent\n";
    return echecs == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
