#include "Application.h"

#include "Constantes.h"
#include "Palette.h"
#include "Ressources.h"
#include "Texte.h"
#include "Touches.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {

constexpr float LARGEUR_LOGIQUE = static_cast<float>(cst::FENETRE_LARGEUR);
constexpr float HAUTEUR_LOGIQUE = static_cast<float>(cst::FENETRE_HAUTEUR);

} // namespace

bool Application::Initialiser() {
    if (!ressources::Charger(police, "LiberationSans-Regular.ttf") || !ressources::Charger(tuiles, "tiles.png") ||
        !ressources::Charger(fondJeu, "FondPrincipal.png") || !ressources::Charger(logo, "TetrisLogo.png") ||
        !ressources::Charger(fondMenu, "Fond.png"))
        return false;

    // Refuse des assets remplacés par des images trop petites ou démesurées
    const unsigned maxTexture = sf::Texture::getMaximumSize();
    const auto tailleValide = [&](const sf::Texture& t, unsigned minX, unsigned minY) {
        return t.getSize().x >= minX && t.getSize().y >= minY && t.getSize().x <= maxTexture && t.getSize().y <= maxTexture;
    };
    if (!tailleValide(tuiles, 8 * cst::TUILE, cst::TUILE) || !tailleValide(fondJeu, cst::FENETRE_LARGEUR, cst::FENETRE_HAUTEUR) ||
        !tailleValide(logo, 1, 1) || !tailleValide(fondMenu, 1, 1)) {
        std::cerr << "Assets invalides : dimensions inattendues\n";
        return false;
    }

    if (!tuilesDaltonien.loadFromImage(palette::Recolorer(tuiles.copyToImage(), cst::TUILE))) {
        std::cerr << "Impossible de préparer la palette pour daltoniens\n";
        return false;
    }

    // Lissage pour les grandes images mises à l'échelle ; pas pour les tuiles (bords nets, pas de débordement)
    fondJeu.setSmooth(true);
    logo.setSmooth(true);
    fondMenu.setSmooth(true);

    cheminReglages = reglages::CheminFichier();
    reglages = reglages::Charger(cheminReglages, touches::ParDefaut(), touches::Code);
    DefinirLangue(reglages.langue);

    AppliquerPleinEcran();
    return true;
}

void Application::SauverReglages() {
    if (!reglages::Sauvegarder(cheminReglages, reglages, touches::Identifiant))
        std::cerr << "Impossible d'enregistrer les réglages dans " << cheminReglages.string() << '\n';
}

void Application::AppliquerPleinEcran() {
    const sf::VideoMode bureau = sf::VideoMode::getDesktopMode();

    if (reglages.pleinEcran) {
        fenetre.create(bureau, "Tetris", sf::State::Fullscreen);
    } else {
        // Plus grande taille (par pas de 0,25) qui tient dans 80 % de l'écran
        const sf::Vector2f ecran(bureau.size);
        float facteur = std::min(ecran.x * 0.8f / LARGEUR_LOGIQUE, ecran.y * 0.8f / HAUTEUR_LOGIQUE);
        facteur = std::max(1.f, std::floor(facteur * 4.f) / 4.f);
        const sf::Vector2u taille(static_cast<unsigned>(LARGEUR_LOGIQUE * facteur),
                                  static_cast<unsigned>(HAUTEUR_LOGIQUE * facteur));

        fenetre.create(sf::VideoMode(taille), "Tetris", sf::Style::Default);
        if (bureau.size.x > taille.x && bureau.size.y > taille.y)
            fenetre.setPosition(sf::Vector2i(bureau.size - taille) / 2);
    }

    AppliquerSynchroVerticale();
    fenetre.setKeyRepeatEnabled(false); // la répétition est gérée par le jeu (DAS/ARR)
    AjusterVue();
}

void Application::AppliquerSynchroVerticale() {
    fenetre.setVerticalSyncEnabled(reglages.synchroVerticale);
}

void Application::AjusterVue() {
    const sf::Vector2u taille = fenetre.getSize();
    if (taille.x == 0 || taille.y == 0) return; // fenêtre réduite

    const float ratioFenetre = static_cast<float>(taille.x) / static_cast<float>(taille.y);
    sf::Vector2f tailleVue(LARGEUR_LOGIQUE, HAUTEUR_LOGIQUE);
    if (ratioFenetre > LARGEUR_LOGIQUE / HAUTEUR_LOGIQUE)
        tailleVue.x = HAUTEUR_LOGIQUE * ratioFenetre; // plus large : on montre plus de décor sur les côtés
    else
        tailleVue.y = LARGEUR_LOGIQUE / ratioFenetre; // plus haute : plus de décor en haut et en bas

    fenetre.setView(sf::View(sf::Vector2f(LARGEUR_LOGIQUE / 2.f, HAUTEUR_LOGIQUE / 2.f), tailleVue));
}

bool Application::GererEvenement(const sf::Event& evenement) {
    if (evenement.is<sf::Event::Closed>()) {
        fenetre.close();
        return true;
    }
    if (evenement.is<sf::Event::Resized>()) {
        AjusterVue();
        return true;
    }
    // Focus suivi ici, mais laissé aussi aux boucles qui veulent y réagir
    if (evenement.is<sf::Event::FocusGained>()) aLeFocus = true;
    if (evenement.is<sf::Event::FocusLost>()) aLeFocus = false;

    const auto* touche = evenement.getIf<sf::Event::KeyPressed>();
    if (!touche || touche->code != sf::Keyboard::Key::F11) return false;
    reglages.pleinEcran = !reglages.pleinEcran;
    AppliquerPleinEcran();
    SauverReglages();
    return true;
}

void Application::Afficher() {
    fenetre.display();

    float cadence = reglages.limiteImages > 0 ? static_cast<float>(reglages.limiteImages) : 300.f;
    if (!aLeFocus) cadence = std::min(cadence, 30.f);

    // Échéances fixes plutôt que « dormir après chaque image » : pas de dérive, cadence régulière
    const sf::Time maintenant = horlogeImage.getElapsedTime();
    prochaineImage += sf::seconds(1.f / cadence);
    if (prochaineImage <= maintenant)
        prochaineImage = maintenant; // en retard (synchro, menu en attente...) : on repart d'ici sans rattraper
    else
        sf::sleep(prochaineImage - maintenant);
}

float Application::Echelle() const {
    const unsigned largeur = fenetre.getSize().x;
    return largeur == 0 ? 1.f : static_cast<float>(largeur) / fenetre.getView().getSize().x; // largeur 0 : fenêtre réduite
}

sf::FloatRect Application::ZoneVisible() const {
    const sf::View& vue = fenetre.getView();
    return {vue.getCenter() - vue.getSize() / 2.f, vue.getSize()};
}

const sf::Texture& Application::Capturer(const std::function<void(sf::RenderTarget&)>& dessiner) {
    sf::Vector2u taille = fenetre.getSize();
    if (taille.x == 0 || taille.y == 0) taille = {cst::FENETRE_LARGEUR, cst::FENETRE_HAUTEUR}; // fenêtre réduite

    if (scene.getSize() != taille) {
        if (!scene.resize(taille)) {
            std::cerr << "Impossible de créer la texture de capture\n";
            return scene.getTexture();
        }
        scene.setSmooth(true); // réduction propre avant le flou
    }
    scene.setView(fenetre.getView());
    scene.clear(COULEUR_FOND);
    dessiner(scene);
    scene.display();
    return scene.getTexture();
}
