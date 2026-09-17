#include "Application.h"
#include "Constantes.h"
#include "Jeu.h"
#include "MeilleurScore.h"
#include "Menu.h"
#include "Rendu.h"
#include "Repetition.h"

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>

namespace {

// Touches tenues, avec répétition gérée par le jeu (la répétition du système est désactivée)
struct Controles {
    RepetitionTouche horizontal;
    RepetitionTouche descente;
    bool gaucheTenue = false;
    bool droiteTenue = false;
    int direction = 0; // -1 gauche, +1 droite : la dernière touche appuyée gagne

    explicit Controles(const Reglages& r)
        : horizontal(static_cast<float>(r.dasMs) / 1000.f, static_cast<float>(r.arrMs) / 1000.f, cst::LARGEUR),
          descente(static_cast<float>(r.descenteDouceMs) / 1000.f, static_cast<float>(r.descenteDouceMs) / 1000.f,
                   cst::HAUTEUR) {}

    void Presser(int dir) {
        (dir < 0 ? gaucheTenue : droiteTenue) = true;
        direction = dir;
        horizontal.Appuyer();
    }

    void Lacher(int dir) {
        (dir < 0 ? gaucheTenue : droiteTenue) = false;
        if (direction != dir) return;
        if (dir < 0 ? droiteTenue : gaucheTenue) {
            direction = -dir; // l'autre touche est encore tenue
            horizontal.Appuyer();
        } else {
            direction = 0;
            horizontal.Relacher();
        }
    }
};

Menu::Choix JouerPartie(Application& app, Menu& menu, Rendu& rendu, const std::filesystem::path& cheminRecord,
                        long long& record) {
    Jeu jeu;
    Controles controles(app.reglages);
    sf::Clock horloge, animation;
    bool abandon = false;

    const auto dessiner = [&](sf::RenderTarget& cible) {
        rendu.Dessiner(cible, jeu, animation.getElapsedTime().asSeconds(), app.Echelle(), app.reglages);
    };

    // Après une pause ou un refus d'abandon : réglages éventuellement modifiés, touches relâchées, 3-2-1
    const auto reprendre = [&] {
        controles = Controles(app.reglages);
        jeu.DefinirDelaiVerrouillage(static_cast<float>(app.reglages.verrouillageMs) / 1000.f);
        menu.CompteARebours(app.Capturer(dessiner));
        horloge.restart();
    };

    jeu.DefinirDelaiVerrouillage(static_cast<float>(app.reglages.verrouillageMs) / 1000.f);

    while (!jeu.Perdu() && !abandon && app.fenetre.isOpen()) {
        // Borné pour éviter un saut de plusieurs cases après un blocage de la fenêtre
        const float dt = std::min(horloge.restart().asSeconds(), 0.25f);
        bool pause = false;
        bool demandeAbandon = false;

        sf::Event evenement;
        while (app.fenetre.pollEvent(evenement)) {
            if (app.GererEvenement(evenement)) continue;

            if (evenement.type == sf::Event::LostFocus) {
                pause = true;
                continue;
            }
            if (evenement.type != sf::Event::KeyPressed && evenement.type != sf::Event::KeyReleased) continue;

            std::optional<Action> action = app.reglages.ActionDe(evenement.key.code);
            // Échap met toujours en pause, sauf si le joueur l'a attribuée à autre chose
            if (!action && evenement.key.code == sf::Keyboard::Escape) action = Action::Pause;
            if (!action) continue;

            if (evenement.type == sf::Event::KeyPressed) {
                switch (*action) {
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
                switch (*action) {
                    case Action::Gauche:        controles.Lacher(-1);            break;
                    case Action::Droite:        controles.Lacher(1);             break;
                    case Action::DescenteDouce: controles.descente.Relacher();   break;
                    default: break;
                }
            }
        }
        if (!app.fenetre.isOpen()) break;

        if (pause || demandeAbandon) {
            const sf::Texture& scene = app.Capturer(dessiner);
            abandon = pause ? menu.Pause(scene) : menu.Confirmer(scene, "Abandonner la partie ?");
            if (!abandon && app.fenetre.isOpen()) reprendre();
            continue;
        }

        // Entrées appliquées avant la gravité, dans la même image
        for (int i = controles.horizontal.MettreAJour(dt); i > 0; i--)
            if (!jeu.Deplacer(controles.direction)) break;
        for (int i = controles.descente.MettreAJour(dt); i > 0; i--)
            if (!jeu.DescenteDouce()) break;
        jeu.MettreAJour(dt);

        app.fenetre.clear(COULEUR_FOND);
        dessiner(app.fenetre);
        app.Afficher();
    }

    if (!app.fenetre.isOpen()) return Menu::Choix::Quitter;

    const bool nouveauRecord = jeu.Score() > record;
    if (nouveauRecord) {
        record = jeu.Score();
        if (!meilleur_score::Sauvegarder(cheminRecord, record))
            std::cerr << "Impossible d'enregistrer le meilleur score dans " << cheminRecord.string() << '\n';
    }

    return menu.Perdu(app.Capturer(dessiner), jeu.Score(), record, nouveauRecord);
}

} // namespace

int main() {
    Application app;
    if (!app.Initialiser()) return EXIT_FAILURE;

    const auto cheminRecord = meilleur_score::CheminFichier();
    long long record = meilleur_score::Charger(cheminRecord);

    Menu menu(app);
    Rendu rendu(app.tuiles, app.fondJeu, app.police);

    bool afficherMenu = true;
    while (app.fenetre.isOpen()) {
        if (afficherMenu && menu.Principal(record) == Menu::Choix::Quitter) break;

        const Menu::Choix choix = JouerPartie(app, menu, rendu, cheminRecord, record);
        if (choix == Menu::Choix::Quitter) break;
        afficherMenu = (choix == Menu::Choix::MenuPrincipal);
    }
    return EXIT_SUCCESS;
}
