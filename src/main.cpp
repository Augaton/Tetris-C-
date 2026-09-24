#include "Application.h"
#include "Classements.h"
#include "Constantes.h"
#include "Effets.h"
#include "Jeu.h"
#include "Manette.h"
#include "MeilleurScore.h"
#include "Menu.h"
#include "Rendu.h"
#include "Repetition.h"
#include "Texte.h"
#include "Touches.h"

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <functional>
#include <iostream>
#include <optional>
#include <random>

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#endif

namespace {

using Touche = sf::Keyboard::Key;

// Vrai si la touche du clavier ou l'entrée de manette est enfoncée en ce moment
bool EstEnfonce(int code) {
    if (manette::EstManette(code)) return manette::EstEnfonce(code);
    return code >= 0 && code < static_cast<int>(sf::Keyboard::KeyCount) && sf::Keyboard::isKeyPressed(static_cast<Touche>(code));
}

// Actions tenues, avec répétition gérée par le jeu (la répétition du système est désactivée)
struct Controles {
    RepetitionTouche horizontal;
    RepetitionTouche descente;
    std::array<bool, NB_ACTIONS> tenues{};
    int direction = 0; // -1 gauche, +1 droite : la dernière appuyée gagne

    explicit Controles(const Reglages& r)
        : horizontal(static_cast<float>(r.dasMs) / 1000.f, static_cast<float>(r.arrMs) / 1000.f, cst::LARGEUR),
          descente(static_cast<float>(r.descenteDouceMs) / 1000.f, static_cast<float>(r.descenteDouceMs) / 1000.f,
                   cst::HAUTEUR) {}

    bool Tenue(Action a) const { return tenues[static_cast<size_t>(a)]; }

    void Presser(int dir) {
        direction = dir;
        horizontal.Appuyer();
    }

    void Lacher(int dir) {
        if (direction != dir) return;
        if (Tenue(dir < 0 ? Action::Droite : Action::Gauche)) {
            direction = -dir; // l'autre direction est encore tenue
            horizontal.Appuyer();
        } else {
            direction = 0;
            horizontal.Relacher();
        }
    }

    // Après une pause : reprend en compte les touches et boutons encore enfoncés (leurs appuis
    // ont été reçus par le menu), pour que la pièce reparte sans relâcher puis réappuyer.
    void Resynchroniser(const Reglages& r) {
        for (int a = 0; a < NB_ACTIONS; a++) {
            bool enfoncee = false;
            for (int code : r.Touches(static_cast<Action>(a))) enfoncee |= code != AUCUNE_TOUCHE && EstEnfonce(code);
            for (int code : r.Boutons(static_cast<Action>(a))) enfoncee |= code != AUCUNE_TOUCHE && EstEnfonce(code);
            tenues[static_cast<size_t>(a)] = enfoncee;
        }
        if (Tenue(Action::Droite)) Presser(1);
        else if (Tenue(Action::Gauche)) Presser(-1);
        if (Tenue(Action::DescenteDouce)) descente.Appuyer();
    }
};

// Revisionnage : rejoue le journal d'une partie en temps réel, avec pause et vitesse réglable
void RevoirPartie(Application& app, Menu& menu, Rendu& rendu, const Enregistrement& journal) {
    Jeu jeu(journal.graine, journal.parametres);
    Effets effets(app.police);
    rendu.DefinirRecord(0);

    size_t suivante = 0;
    double reserve = 0.0;  // temps de jeu disponible pour avancer dans le journal
    float tempsJeu = 0.f;  // horloge des animations, au rythme de la partie
    float vitesse = 1.f;
    bool enPause = false;

    sf::Text bandeau(app.police);
    bandeau.setStyle(sf::Text::Bold);
    sf::Clock horloge;

    while (app.fenetre.isOpen()) {
        const float dt = std::min(horloge.restart().asSeconds(), 0.25f);
        const bool fini = suivante >= journal.commandes.size();

        while (const std::optional evenement = menu.Lire()) {
            if (app.GererEvenement(*evenement)) continue;
            if (evenement->is<sf::Event::MouseButtonPressed>() && fini) return;
            const auto* touche = evenement->getIf<sf::Event::KeyPressed>();
            if (!touche) continue;
            switch (touche->code) {
                case Touche::Escape:
                case Touche::Backspace: return;
                case Touche::Enter:
                    if (fini) return;
                    enPause = !enPause;
                    break;
                case Touche::Space:
                case Touche::P: enPause = !enPause; break;
                case Touche::Left:
                case Touche::Down: vitesse = std::max(0.25f, vitesse / 2.f); break;
                case Touche::Right:
                case Touche::Up: vitesse = std::min(8.f, vitesse * 2.f); break;
                default: break;
            }
        }
        if (!app.fenetre.isOpen()) break;

        if (!enPause) reserve += static_cast<double>(dt * vitesse);
        while (suivante < journal.commandes.size()) {
            const Commande& c = journal.commandes[suivante];
            if (c.type == Commande::Type::Temps) {
                if (reserve < static_cast<double>(c.valeur)) break;
                reserve -= static_cast<double>(c.valeur);
                jeu.Rejouer(c);
                effets.Traiter(jeu.Evenements(), app.reglages);
                jeu.ViderEvenements();
                effets.MettreAJour(c.valeur);
                tempsJeu += c.valeur;
            } else {
                jeu.Rejouer(c);
            }
            suivante++;
        }
        if (suivante >= journal.commandes.size()) reserve = 0.0;

        app.fenetre.clear(theme::FOND_BAS);
        rendu.Dessiner(app.fenetre, jeu, tempsJeu, app.Echelle(), app.reglages, effets);

        const sf::String etat = suivante >= journal.commandes.size()
                                    ? TrU("FIN DU REPLAY · Entrée ou Échap pour revenir", "END OF REPLAY · Enter or Esc to go back")
                                    : TrU("REPLAY ", "REPLAY ") + Utf8(std::format("×{:g}", vitesse)) +
                                          (enPause ? TrU(" · EN PAUSE", " · PAUSED") : sf::String()) +
                                          TrU(" · Espace pause · ←→ vitesse · Échap quitter",
                                              " · Space pause · ←→ speed · Esc exit");
        bandeau.setString(etat);
        bandeau.setFillColor(sf::Color(255, 204, 0));
        PlacerTexteBorne(bandeau, 15, {static_cast<float>(cst::FENETRE_LARGEUR) / 2.f, 527.f}, app.Echelle(),
                         app.ZoneVisible().size.x - 20.f);
        app.fenetre.draw(bandeau);
        app.Afficher();
    }
}

Menu::Choix JouerPartie(Application& app, Menu& menu, Rendu& rendu, Classements& classements,
                        const std::filesystem::path& cheminClassements, const ParametresPartie& parametres) {
    Jeu jeu(std::random_device{}(), parametres);
    jeu.ActiverEnregistrement();
    Effets effets(app.police);

    // Record à battre (le Sprint se joue au temps : pas de score doré)
    const EntreeClassement* premier = classements.Premier(parametres.mode);
    const long long record = parametres.mode != Mode::Sprint && premier ? premier->score : 0;
    rendu.DefinirRecord(record);
    bool recordAnnonce = record <= 0; // pas d'annonce pour la toute première partie

    Controles controles(app.reglages);
    manette::Traducteur traducteur;
    sf::Clock horloge, animation;
    bool abandon = false;
    int pieceTraitee = jeu.NumeroPiece();

    const auto dessiner = [&](sf::RenderTarget& cible) {
        rendu.Dessiner(cible, jeu, animation.getElapsedTime().asSeconds(), app.Echelle(), app.reglages, effets);
    };

    // Après une pause ou un refus d'abandon : réglages éventuellement modifiés, touches relâchées, 3-2-1
    const auto reprendre = [&] {
        controles = Controles(app.reglages);
        jeu.DefinirDelaiVerrouillage(static_cast<float>(app.reglages.verrouillageMs) / 1000.f);
        menu.CompteARebours(app.Capturer(dessiner));
        controles.Resynchroniser(app.reglages);
        horloge.restart();
    };

    jeu.DefinirDelaiVerrouillage(static_cast<float>(app.reglages.verrouillageMs) / 1000.f);

    while (!jeu.Fini() && !abandon && app.fenetre.isOpen()) {
        // Borné pour éviter un saut de plusieurs cases après un blocage de la fenêtre
        const float dt = std::min(horloge.restart().asSeconds(), 0.25f);
        bool pause = false;
        bool demandeAbandon = false;

        const auto appliquer = [&](Action action, bool appui) {
            controles.tenues[static_cast<size_t>(action)] = appui;
            if (appui) {
                switch (action) {
                    case Action::Gauche:             controles.Presser(-1);         break;
                    case Action::Droite:             controles.Presser(1);          break;
                    case Action::DescenteDouce:      controles.descente.Appuyer();  break;
                    case Action::ChuteRapide:        jeu.ChuteRapide();             break;
                    case Action::TournerHoraire:     jeu.Tourner(true);             break;
                    case Action::TournerAntiHoraire: jeu.Tourner(false);            break;
                    case Action::Garder:             jeu.Garder();                  break;
                    case Action::Pause:              pause = true;                  break;
                    case Action::Abandonner:         demandeAbandon = true;         break;
                }
            } else {
                switch (action) {
                    case Action::Gauche:        controles.Lacher(-1);            break;
                    case Action::Droite:        controles.Lacher(1);             break;
                    case Action::DescenteDouce: controles.descente.Relacher();   break;
                    default: break;
                }
            }
        };

        while (const std::optional evenement = app.fenetre.pollEvent()) {
            if (app.GererEvenement(*evenement)) continue;

            if (evenement->is<sf::Event::FocusLost>()) {
                // Ignoré si le focus est déjà revenu (ex. fenêtre recréée par F11)
                if (!app.fenetre.hasFocus()) pause = true;
                continue;
            }

            const auto* appui = evenement->getIf<sf::Event::KeyPressed>();
            const auto* relache = evenement->getIf<sf::Event::KeyReleased>();
            if (appui || relache) {
                const Touche touche = appui ? appui->code : relache->code;
                std::optional<Action> action = app.reglages.ActionDe(touches::CodeTouche(touche));
                // Échap met toujours en pause, sauf si le joueur l'a attribuée à autre chose
                if (!action && touche == Touche::Escape) action = Action::Pause;
                if (action) appliquer(*action, appui != nullptr);
                continue;
            }

            for (const manette::Entree& entree : traducteur.Traduire(*evenement))
                if (auto action = app.reglages.ActionDe(entree.code)) appliquer(*action, entree.appui);
        }
        if (!app.fenetre.isOpen()) break;

        if (pause || demandeAbandon) {
            const sf::Texture& scene = app.Capturer(dessiner);
            abandon = pause ? menu.Pause(scene) : menu.Confirmer(scene, Tr("Abandonner la partie ?", "Quit this game?"));
            if (!abandon && app.fenetre.isOpen()) reprendre();
            continue;
        }

        // Entrées appliquées avant la gravité, dans la même image
        for (int i = controles.horizontal.MettreAJour(dt); i > 0; i--)
            if (!jeu.Deplacer(controles.direction)) break;
        for (int i = controles.descente.MettreAJour(dt); i > 0; i--)
            if (!jeu.DescenteDouce()) break;
        jeu.MettreAJour(dt);

        // Rotation et garde anticipées (IRS/IHS) : actions déjà tenues quand la nouvelle pièce apparaît
        if (jeu.NumeroPiece() != pieceTraitee && app.reglages.rotationAnticipee && !jeu.Fini()) {
            if (controles.Tenue(Action::Garder)) jeu.Garder();
            if (controles.Tenue(Action::TournerHoraire)) jeu.Tourner(true);
            else if (controles.Tenue(Action::TournerAntiHoraire)) jeu.Tourner(false);
        }
        pieceTraitee = jeu.NumeroPiece();

        effets.Traiter(jeu.Evenements(), app.reglages);
        if (!recordAnnonce && jeu.Score() > record) {
            recordAnnonce = true;
            if (app.reglages.effets) effets.AnnoncerRecord();
        }
        jeu.ViderEvenements();
        effets.MettreAJour(dt);

        app.fenetre.clear(theme::FOND_BAS);
        dessiner(app.fenetre);
        app.Afficher();
    }

    // Classement, enregistré même si la fenêtre a été fermée en pleine partie.
    // Sprint : seulement s'il est terminé ; autres modes : dès qu'il y a des points.
    ResumePartie resume;
    resume.parametres = parametres;
    resume.perdu = jeu.Perdu();
    resume.objectifAtteint = jeu.ObjectifAtteint();
    resume.score = jeu.Score();
    resume.lignes = jeu.Lignes();
    resume.niveau = jeu.Niveau();
    resume.temps = jeu.Temps();
    resume.stats = jeu.Stats();

    const bool classable = parametres.mode == Mode::Sprint ? jeu.ObjectifAtteint() : jeu.Score() > 0;
    if (classable) {
        const EntreeClassement entree{jeu.Score(), jeu.Lignes(), static_cast<int>(std::lround(jeu.Temps() * 1000.f)),
                                      classements::DateActuelle()};
        resume.rang = classements.Ajouter(parametres.mode, entree);
        if (resume.rang >= 0 && !classements::Sauvegarder(cheminClassements, classements))
            std::cerr << "Impossible d'enregistrer les classements dans " << cheminClassements.string() << '\n';
    }
    if (const EntreeClassement* meilleur = classements.Premier(parametres.mode)) resume.premier = *meilleur;

    if (!app.fenetre.isOpen()) return Menu::Choix::Quitter;

    std::function<void()> revoir;
    if (jeu.Journal().complet) revoir = [&] { RevoirPartie(app, menu, rendu, jeu.Journal()); };
    return menu.FinDePartie(app.Capturer(dessiner), resume, revoir);
}

} // namespace

int main() {
#ifdef _WIN32
    // Retire le dossier courant de la recherche des DLL (détournement de DLL)
    SetDllDirectoryW(L"");
#endif

    Application app;
    if (!app.Initialiser()) return EXIT_FAILURE;

    const auto cheminClassements = classements::CheminFichier();
    Classements classements = classements::Charger(cheminClassements);

    // Reprise de l'ancien meilleur score (avant les classements) dans le top du Marathon
    if (classements.Table(Mode::Marathon).empty()) {
        const long long ancien = meilleur_score::Charger(meilleur_score::CheminFichier());
        if (ancien > 0) {
            classements.Ajouter(Mode::Marathon, {ancien, 0, 0, ""});
            classements::Sauvegarder(cheminClassements, classements);
        }
    }

    Menu menu(app);
    Rendu rendu(app.police);

    ParametresPartie parametres{app.reglages.mode, app.reglages.niveauDepart};
    bool afficherMenu = true;
    while (app.fenetre.isOpen()) {
        if (afficherMenu && menu.Principal(classements, parametres) == Menu::Choix::Quitter) break;

        const Menu::Choix choix = JouerPartie(app, menu, rendu, classements, cheminClassements, parametres);
        if (choix == Menu::Choix::Quitter) break;
        afficherMenu = (choix == Menu::Choix::MenuPrincipal);
    }
    return EXIT_SUCCESS;
}
