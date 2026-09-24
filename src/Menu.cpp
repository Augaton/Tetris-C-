#include "Menu.h"

#include "Constantes.h"
#include "Formes.h"
#include "Palette.h"
#include "Texte.h"
#include "Touches.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

using Touche = sf::Keyboard::Key;

// Flou gaussien séparable : une passe horizontale puis une verticale,
// 11 lectures par pixel par passe au lieu de 121 (ou 625) pour un flou 2D direct
constexpr std::string_view SHADER_FLOU = R"(
uniform sampler2D texture;
uniform vec2 direction; // taille d'un pixel dans la direction du flou

void main() {
    vec2 uv = gl_TexCoord[0].xy;
    vec4 couleur = vec4(0.0);
    float total = 0.0;
    for (int i = -5; i <= 5; i++) {
        float x = float(i);
        float poids = exp(-(x * x) / 12.5); // sigma = 2,5
        couleur += texture2D(texture, uv + direction * x) * poids;
        total += poids;
    }
    gl_FragColor = couleur / total;
})";

// Le flou est calculé à cette largeur au plus : son coût ne dépend plus de la résolution de l'écran
constexpr float LARGEUR_FLOU = static_cast<float>(cst::FENETRE_LARGEUR);

// Ne recrée la texture que si sa taille change
bool Dimensionner(sf::RenderTexture& texture, sf::Vector2u taille) {
    if (texture.getSize() == taille) return true;
    if (!texture.resize(taille)) return false;
    texture.setSmooth(true);
    return true;
}

constexpr float CENTRE_X = static_cast<float>(cst::FENETRE_LARGEUR) / 2.f;
constexpr float CENTRE_Y = static_cast<float>(cst::FENETRE_HAUTEUR) / 2.f;
constexpr sf::Color JAUNE(255, 204, 0);
constexpr sf::Color GRIS(170, 170, 170);

unsigned Taille(unsigned taille, float facteur) {
    return static_cast<unsigned>(std::lround(static_cast<float>(taille) * facteur));
}

struct StyleListe {
    bool majuscules = false; // boutons (menus principaux) ou phrases (réglages)
    bool panneau = true;
};

// Liste de boutons au style Tetris minimal : panneau sombre arrondi, surlignage qui glisse d'une
// entrée à l'autre avec une mini-tuile de couleur, fondu à l'ouverture.
// Flèches haut/bas pour choisir, Entrée/Espace/clic pour valider ; flèches gauche/droite,
// clic droit ou molette pour modifier une valeur.
// Chaque ligne garde son propre texte : la mise en page n'est refaite que si elle change.
class Liste {
public:
    struct Resultat {
        int activee = -1; // entrée validée
        int delta = 0;    // modification demandée sur l'entrée sélectionnée
    };
    Liste(Application& application, float hautListe, float ecart, unsigned tailleTexte, float largeurMax = 560.f,
          StyleListe apparence = {})
        : app(application), haut(hautListe), espacement(ecart), taille(tailleTexte), largeur(largeurMax), style(apparence) {
        formes.resize(6 * 24 * 4);
        formes.clear();
    }

    void Definir(std::vector<sf::String> nouvelles) {
        // Les textes ont besoin de la police : les lignes ajoutées sont construites avec elle
        if (lignes.size() != nouvelles.size()) lignes.resize(nouvelles.size(), Ligne(app.police));
        for (size_t i = 0; i < nouvelles.size(); i++) {
            const sf::String chaine = style.majuscules ? Majuscules(nouvelles[i]) : nouvelles[i];
            if (chaine != lignes[i].chaine) {
                lignes[i].chaine = chaine;
                lignes[i].echelle = 0.f; // à remettre en page
            }
        }
        if (!lignes.empty()) selection = std::clamp(selection, 0, static_cast<int>(lignes.size()) - 1);
    }
    int Selection() const { return selection; }
    void Selectionner(int indice) { selection = indice; }

    // Vrai tant que le fondu d'ouverture ou le glissement du surlignage n'est pas terminé
    bool EnAnimation() const {
        return apparition < 1.f || (ySurlignage >= 0.f && std::abs(ySurlignage - Centre(selection).y) > 0.3f);
    }

    Resultat Traiter(const sf::Event& e) {
        const int n = static_cast<int>(lignes.size());
        if (n == 0) return {};

        if (const auto* touche = e.getIf<sf::Event::KeyPressed>()) {
            switch (touche->code) {
                case Touche::Up:    selection = (selection + n - 1) % n; break;
                case Touche::Down:  selection = (selection + 1) % n;     break;
                case Touche::Enter:
                case Touche::Space: return {selection, 0};
                case Touche::Left:  return {-1, -1};
                case Touche::Right: return {-1, 1};
                default: break;
            }
        } else if (const auto* souris = e.getIf<sf::Event::MouseMoved>()) {
            const int ligne = LigneSous(souris->position);
            if (ligne >= 0) selection = ligne;
        } else if (const auto* clic = e.getIf<sf::Event::MouseButtonPressed>()) {
            const int ligne = LigneSous(clic->position);
            if (ligne < 0) return {};
            selection = ligne;
            if (clic->button == sf::Mouse::Button::Left) return {ligne, 0};
            if (clic->button == sf::Mouse::Button::Right) return {-1, -1};
        } else if (const auto* molette = e.getIf<sf::Event::MouseWheelScrolled>()) {
            const int ligne = LigneSous(molette->position);
            if (ligne < 0 || molette->delta == 0.f) return {};
            selection = ligne;
            return {-1, molette->delta > 0.f ? 1 : -1};
        }
        return {};
    }

    void Dessiner() {
        if (lignes.empty()) return;
        const float dt = std::min(horloge.restart().asSeconds(), 0.05f);
        const float echelle = app.Echelle();
        const unsigned tailleTexte = Taille(taille, app.FacteurTexte());

        // Mise en page des textes qui ont changé (chaîne, échelle ou taille)
        float largeurMax = 0.f;
        for (Ligne& l : lignes) {
            if (l.echelle != echelle || l.taille != tailleTexte) {
                l.echelle = echelle;
                l.taille = tailleTexte;
                l.texte.setStyle(sf::Text::Bold);
                l.texte.setLetterSpacing(style.majuscules ? 1.6f : 1.f);
                l.texte.setString(l.chaine);
                PlacerTexte(l.texte, tailleTexte, {0.f, 0.f}, echelle);
                const float largeurLigne = l.texte.getGlobalBounds().size.x;
                // Place réservée de chaque côté pour la mini-tuile du surlignage
                if (largeurLigne > largeur - 110.f)
                    PlacerTexte(l.texte, tailleTexte, {0.f, 0.f}, echelle, (largeur - 110.f) / largeurLigne);
                l.echelleBase = l.texte.getScale().x;
                l.largeur = l.texte.getGlobalBounds().size.x;
            }
            largeurMax = std::max(largeurMax, l.largeur);
        }

        // Ouverture : fondu et léger glissement vers le haut (courbe « ease-out »)
        apparition = std::min(1.f, apparition + dt / 0.2f);
        const float a = 1.f - std::pow(1.f - apparition, 3.f);
        const float glissement = (1.f - a) * 14.f;

        // Le surlignage rattrape l'entrée choisie
        const float cible = Centre(selection).y;
        if (ySurlignage < 0.f) ySurlignage = cible;
        else ySurlignage += (cible - ySurlignage) * (1.f - std::exp(-20.f * dt));

        const float largeurPanneau = std::clamp(largeurMax + 110.f, 260.f, largeur);
        const float gauche = CENTRE_X - largeurPanneau / 2.f;
        const bool daltonien = app.reglages.daltonien;
        const int couleurTuile = selection % 7 + 1;
        const sf::Color accent = palette::Tuile(couleurTuile, daltonien);

        formes.clear();
        if (style.panneau) {
            const float hauteurPanneau = espacement * static_cast<float>(lignes.size()) + 20.f;
            formes::AjouterRectangleArrondi(formes, {{gauche, haut - espacement / 2.f - 10.f + glissement}, {largeurPanneau, hauteurPanneau}},
                                            14.f, sf::Color(14, 14, 14, static_cast<std::uint8_t>(205.f * a)));
        }
        const float hauteurBarre = espacement - 8.f;
        const sf::FloatRect barre({gauche + 8.f, ySurlignage - hauteurBarre / 2.f + glissement}, {largeurPanneau - 16.f, hauteurBarre});
        formes::AjouterRectangleArrondi(formes, barre, hauteurBarre / 2.f,
                                        sf::Color(accent.r, accent.g, accent.b, static_cast<std::uint8_t>(48.f * a)));
        app.fenetre.draw(formes);

        // Mini-tuile du jeu devant l'entrée choisie
        const float cote = std::min(14.f, hauteurBarre - 8.f);
        tuile.setTexture(daltonien ? app.tuilesDaltonien : app.tuiles);
        tuile.setTextureRect({{cst::TUILE * couleurTuile, 0}, {cst::TUILE, cst::TUILE}});
        tuile.setScale({cote / cst::TUILE, cote / cst::TUILE});
        tuile.setPosition({barre.position.x + hauteurBarre / 2.f - cote / 2.f + 2.f, ySurlignage - cote / 2.f + glissement});
        tuile.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(255.f * a)));
        app.fenetre.draw(tuile);

        for (size_t i = 0; i < lignes.size(); i++) {
            Ligne& l = lignes[i];
            const bool choisie = static_cast<int>(i) == selection;
            const auto alpha = static_cast<std::uint8_t>(255.f * a);
            l.texte.setFillColor(choisie ? sf::Color(255, 255, 255, alpha) : sf::Color(165, 165, 165, alpha));
            const float zoom = choisie ? 1.04f : 1.f;
            l.texte.setScale({l.echelleBase * zoom, l.echelleBase * zoom});
            l.texte.setPosition({CENTRE_X, Centre(static_cast<int>(i)).y + glissement});
            app.fenetre.draw(l.texte);
        }
    }

private:
    struct Ligne {
        explicit Ligne(const sf::Font& police) : texte(police) {}
        sf::Text texte;
        sf::String chaine;
        float echelle = 0.f;
        unsigned taille = 0;
        float echelleBase = 1.f;
        float largeur = 0.f;
    };

    Application& app;
    float haut, espacement;
    unsigned taille;
    float largeur;
    StyleListe style;
    std::vector<Ligne> lignes;
    int selection = 0;
    float ySurlignage = -1.f; // position affichée du surlignage
    float apparition = 0.f;   // 0 → 1 à l'ouverture
    sf::Clock horloge;
    sf::VertexArray formes{sf::PrimitiveType::Triangles};
    sf::Sprite tuile{app.tuiles};

    sf::Vector2f Centre(int i) const { return {CENTRE_X, haut + espacement * static_cast<float>(i)}; }

    int LigneSous(sf::Vector2i pixel) const {
        const sf::Vector2f p = app.fenetre.mapPixelToCoords(pixel);
        if (std::abs(p.x - CENTRE_X) > largeur / 2.f) return -1;
        for (size_t i = 0; i < lignes.size(); i++)
            if (std::abs(p.y - Centre(static_cast<int>(i)).y) <= espacement / 2.f) return static_cast<int>(i);
        return -1;
    }
};

// Touche appuyée par cet événement, Unknown si ce n'est pas un appui de touche
Touche Appui(const sf::Event& e) {
    const auto* touche = e.getIf<sf::Event::KeyPressed>();
    return touche ? touche->code : Touche::Unknown;
}

bool Echap(const sf::Event& e) {
    return Appui(e) == Touche::Escape;
}

void Ajuster(int& valeur, const Bornes& bornes, int delta, bool boucler) {
    int nouvelle = valeur + delta * bornes.pas;
    if (boucler && nouvelle > bornes.max) nouvelle = bornes.min;
    valeur = bornes.Limiter(nouvelle);
}

sf::String Duree(int ms, const char* siZeroFr, const char* siZeroEn) {
    return ms == 0 ? TrU(siZeroFr, siZeroEn) : Utf8(std::format("{} ms", ms));
}

// Limites proposées pour les images par seconde (0 = automatique)
constexpr std::array LIMITES_IMAGES = {0, 60, 75, 120, 144, 165, 240, 360};

int LimiteVoisine(int valeur, int delta, bool boucler) {
    const int nombre = static_cast<int>(LIMITES_IMAGES.size());
    int indice = 0;
    for (int i = 0; i < nombre; i++)
        if (LIMITES_IMAGES[static_cast<size_t>(i)] <= valeur) indice = i; // valeur saisie à la main : la plus proche en dessous
    indice += delta;
    if (boucler) indice = (indice + nombre) % nombre;
    return LIMITES_IMAGES[static_cast<size_t>(std::clamp(indice, 0, nombre - 1))];
}

sf::String OuiNon(bool valeur) {
    return valeur ? TrU("Oui", "On") : TrU("Non", "Off");
}

sf::String NomMode(Mode m) {
    switch (m) {
        case Mode::Marathon: return Utf8("Marathon");
        case Mode::Sprint:   return Utf8("Sprint");
        case Mode::Ultra:    return Utf8("Ultra");
        case Mode::Zen:      return Utf8("Zen");
    }
    return {};
}

// Valeur qui classe une partie : temps en Sprint, score ailleurs
std::string ValeurClassee(Mode m, const EntreeClassement& e) {
    return m == Mode::Sprint ? FormaterTemps(static_cast<float>(e.tempsMs) / 1000.f) : FormaterNombre(e.score);
}

sf::String TexteRecord(Mode m, const EntreeClassement* premier) {
    if (!premier) return TrU("Pas encore de record", "No record yet");
    return (m == Mode::Sprint ? TrU("Meilleur temps : ", "Best time: ") : TrU("Record : ", "Best score: ")) +
           Utf8(ValeurClassee(m, *premier));
}

std::string Decimal(float valeur) {
    std::string texte = std::format("{:.2f}", valeur);
    if (LangueActuelle() == Langue::Francais) std::ranges::replace(texte, '.', ',');
    return texte;
}

} // namespace

Menu::Menu(Application& application) : app(application) {
    texte.setStyle(sf::Text::Bold);

    // Sans shader (GPU non compatible) on affiche le fond sans flou au lieu de planter
    shaderOk = sf::Shader::isAvailable() && blurShader.loadFromMemory(SHADER_FLOU, sf::Shader::Type::Fragment);
    if (!shaderOk) std::cerr << "Shader de flou indisponible\n";
}

std::optional<sf::Event> Menu::Lire() {
    while (std::optional evenement = app.fenetre.pollEvent()) {
        // La déconnexion passe aussi par le traducteur : il oublie les axes tenus de cette manette
        const bool deManette = evenement->is<sf::Event::JoystickButtonPressed>() ||
                               evenement->is<sf::Event::JoystickButtonReleased>() ||
                               evenement->is<sf::Event::JoystickMoved>() ||
                               evenement->is<sf::Event::JoystickDisconnected>();
        if (!deManette || !traduireManette) return evenement;

        for (const manette::Entree& entree : traducteur.Traduire(*evenement))
            if (auto touche = manette::VersClavier(entree)) return touche;
        // Événement de manette sans effet dans les menus (bruit des sticks) : ignoré, sans redessiner
    }
    return std::nullopt;
}

// Écran immobile déjà affiché : au lieu de le redessiner à chaque image, on attend le prochain
// événement par petites siestes (processeur et GPU quasi au repos), avec un rafraîchissement
// de sécurité chaque seconde.
std::optional<sf::Event> Menu::ProchainEvenement(bool& aJour, bool anime) {
    if (auto evenement = Lire()) {
        aJour = false;
        return evenement;
    }
    if (!aJour || anime) return std::nullopt;

    while (app.fenetre.isOpen()) {
        sf::sleep(sf::milliseconds(8));
        if (auto evenement = Lire()) {
            aJour = false;
            return evenement;
        }
        if (horlogeRepos.getElapsedTime() >= sf::seconds(1.f)) break;
    }
    aJour = false;
    return std::nullopt;
}

void Menu::AfficherImage(bool& aJour) {
    app.Afficher();
    aJour = true;
    horlogeRepos.restart();
}

void Menu::DessinerTexte(const sf::String& chaine, unsigned taille, float y, sf::Color couleur) {
    DessinerTexte(chaine, taille, {CENTRE_X, y}, couleur, app.ZoneVisible().size.x - 20.f);
}

void Menu::DessinerTexte(const sf::String& chaine, unsigned taille, sf::Vector2f centre, sf::Color couleur,
                         float largeurMax) {
    texte.setString(chaine);
    texte.setFillColor(couleur);
    PlacerTexteBorne(texte, Taille(taille, app.FacteurTexte()), centre, app.Echelle(), largeurMax);
    app.fenetre.draw(texte);
}

// Calcule le flou une seule fois par ouverture de menu, en basse résolution
void Menu::PreparerFlou(const sf::Texture& scene) {
    const sf::Vector2u source = scene.getSize();
    if (source.x == 0 || source.y == 0) return;

    const float reduction = std::min(1.f, LARGEUR_FLOU / static_cast<float>(source.x));
    const sf::Vector2u taille(std::max(1u, static_cast<unsigned>(static_cast<float>(source.x) * reduction)),
                              std::max(1u, static_cast<unsigned>(static_cast<float>(source.y) * reduction)));
    if (!Dimensionner(flouReduit, taille) || !Dimensionner(flouIntermediaire, taille) ||
        !Dimensionner(fondFlou, taille))
        return;

    sf::Sprite reduit(scene);
    reduit.setScale(sf::Vector2f(taille).componentWiseDiv(sf::Vector2f(source)));
    flouReduit.clear();
    flouReduit.draw(reduit);
    flouReduit.display();

    fondFlou.clear();
    if (!shaderOk) {
        fondFlou.draw(sf::Sprite(flouReduit.getTexture()));
        fondFlou.display();
        return;
    }

    blurShader.setUniform("texture", sf::Shader::CurrentTexture);
    blurShader.setUniform("direction", sf::Vector2f(1.f / static_cast<float>(taille.x), 0.f));
    flouIntermediaire.clear();
    flouIntermediaire.draw(sf::Sprite(flouReduit.getTexture()), &blurShader);
    flouIntermediaire.display();

    blurShader.setUniform("direction", sf::Vector2f(0.f, 1.f / static_cast<float>(taille.y)));
    fondFlou.draw(sf::Sprite(flouIntermediaire.getTexture()), &blurShader);
    fondFlou.display();
}

// Titre : majuscules espacées, souligné par quatre mini-tuiles aux couleurs des pièces
void Menu::DessinerTitre(const sf::String& chaine, float y) {
    texte.setLetterSpacing(2.5f);
    DessinerTexte(Majuscules(chaine), 32, y);
    texte.setLetterSpacing(1.f);

    decor.clear();
    constexpr std::array couleurs = {5, 4, 1, 3}; // cyan, jaune, violet, vert
    constexpr float cote = 7.f, pas = 11.f;
    for (size_t i = 0; i < couleurs.size(); i++) {
        const sf::Color c = palette::Tuile(couleurs[i], app.reglages.daltonien);
        const float x = CENTRE_X - 2.f * pas + pas * static_cast<float>(i) + (pas - cote) / 2.f;
        formes::AjouterRectangleArrondi(decor, {{x, y + 24.f}, {cote, cote}}, 1.5f, c);
    }
    app.fenetre.draw(decor);
}

// Capture floutée de la partie assombrie d'un voile, pour que les menus restent lisibles par-dessus
void Menu::DessinerFondFlou() {
    DessinerDansZone(fondFlou.getTexture());
    const sf::FloatRect zone = app.ZoneVisible();
    sf::RectangleShape voile(zone.size);
    voile.setPosition(zone.position);
    voile.setFillColor(sf::Color(0, 0, 0, 140));
    app.fenetre.draw(voile);
}

// Les captures couvrent toute la fenêtre : on les étire sur la zone visible actuelle
void Menu::DessinerDansZone(const sf::Texture& texture) {
    if (texture.getSize().x == 0 || texture.getSize().y == 0) return;
    const sf::FloatRect zone = app.ZoneVisible();
    sf::Sprite sprite(texture);
    sprite.setPosition(zone.position);
    sprite.setScale(zone.size.componentWiseDiv(sf::Vector2f(texture.getSize())));
    app.fenetre.draw(sprite);
}

// ---------------------------------------------------------------------------------------------
// Menu principal
// ---------------------------------------------------------------------------------------------

Menu::Choix Menu::Principal(const Classements& classements, ParametresPartie& parametres) {
    Liste liste(app, 262.f, 48.f, 24, 420.f, {true});

    // Fond qui défile vers le bas, répété pour couvrir toute la zone visible
    sf::Sprite spriteFond(app.fondMenu);
    sf::Clock horlogeFond;
    float decalage = 0.f;
    sf::RectangleShape voile; // assombrit le décor : les menus ressortent, style épuré
    voile.setFillColor(sf::Color(8, 8, 8, 170));
    const Fond fond{[&] {
        const sf::FloatRect zone = app.ZoneVisible();
        const sf::Vector2f tailleTexture(app.fondMenu.getSize());
        const float echelleFond = std::max(1.f, zone.size.x / tailleTexture.x);
        const float hauteur = tailleTexture.y * echelleFond;

        decalage = std::fmod(decalage + 18.f * horlogeFond.restart().asSeconds(), hauteur);
        spriteFond.setScale({echelleFond, echelleFond});
        const float x = CENTRE_X - tailleTexture.x * echelleFond / 2.f;
        for (float y = zone.position.y + decalage - hauteur; y < zone.position.y + zone.size.y; y += hauteur) {
            spriteFond.setPosition({x, y});
            app.fenetre.draw(spriteFond);
        }
        voile.setSize(zone.size);
        voile.setPosition(zone.position);
        app.fenetre.draw(voile);
    }, true};

    sf::Sprite spriteLogo(app.logo);
    spriteLogo.setOrigin(spriteLogo.getLocalBounds().size / 2.f);
    spriteLogo.setPosition({CENTRE_X, 100.f});

    const auto remplir = [&] {
        // Reconstruit chaque image : la langue peut changer depuis les options
        liste.Definir({TrU("Jouer !", "Play!"), TrU("Classements", "Leaderboards"), TrU("Options", "Options"),
                       TrU("Commandes", "Controls"), TrU("Quitter", "Quit")});
    };
    while (app.fenetre.isOpen()) {
        remplir();
        while (const std::optional evenement = Lire()) {
            if (app.GererEvenement(*evenement)) continue;
            switch (liste.Traiter(*evenement).activee) {
                case 0:
                    if (auto choix = ChoisirMode(fond, classements)) {
                        parametres = *choix;
                        return Choix::Jouer;
                    }
                    break;
                case 1: AfficherClassements(fond, classements); break;
                case 2: Options(fond); break;
                case 3: Commandes(fond); break;
                case 4: return Choix::Quitter;
                default: break;
            }
        }
        if (!app.fenetre.isOpen()) break;

        remplir();

        app.fenetre.clear(COULEUR_FOND);
        fond.dessiner();
        app.fenetre.draw(spriteLogo);
        liste.Dessiner();
        const Mode dernier = app.reglages.mode;
        DessinerTexte(NomMode(dernier) + Utf8(" · ") + TexteRecord(dernier, classements.Premier(dernier)), 18, 492.f);
        DessinerTexte(TrU("↑↓ choisir · Entrée valider · F11 plein écran", "↑↓ select · Enter confirm · F11 fullscreen"),
                      14, 525.f, GRIS);
        app.Afficher();
    }
    return Choix::Quitter;
}

std::optional<ParametresPartie> Menu::ChoisirMode(const Fond& fond, const Classements& classements) {
    Liste liste(app, 180.f, 50.f, 20, 640.f, {true});
    liste.Selectionner(static_cast<int>(app.reglages.mode));
    Reglages& r = app.reglages;

    bool aJour = false;
    const auto remplir = [&] {
        liste.Definir({
            Utf8("Marathon · ") + TrU("niveau de départ : ", "start level: ") + Utf8(std::format("< {} >", r.niveauDepart)),
            Utf8(std::format("Sprint · {}", mode::SPRINT_LIGNES)) + TrU(" lignes", " lines"),
            Utf8("Ultra · 2 minutes"),
            Utf8("Zen"),
            TrU("Retour", "Back"),
        });
    };
    while (app.fenetre.isOpen()) {
        remplir();
        while (const std::optional evenement = ProchainEvenement(aJour, fond.anime || liste.EnAnimation())) {
            if (app.GererEvenement(*evenement)) continue;
            if (Echap(*evenement)) return std::nullopt;

            const Liste::Resultat res = liste.Traiter(*evenement);
            // Gauche/droite sur Marathon : niveau de départ
            if (res.delta != 0 && liste.Selection() == 0)
                r.niveauDepart = std::clamp(r.niveauDepart + res.delta, 0, mode::NIVEAU_DEPART_MAX);
            if (res.activee < 0) continue;
            if (res.activee == NB_MODES) return std::nullopt;

            r.mode = static_cast<Mode>(res.activee);
            app.SauverReglages(); // mémorise le dernier mode choisi
            return ParametresPartie{r.mode, r.mode == Mode::Marathon ? r.niveauDepart : 0};
        }
        if (!app.fenetre.isOpen()) break;

        remplir();

        static constexpr std::array<std::pair<const char*, const char*>, NB_MODES> descriptions = {{
            {"Un niveau de plus toutes les 10 lignes, jusqu'à la défaite.", "One level up every 10 lines, until you top out."},
            {"Faites 40 lignes le plus vite possible.", "Clear 40 lines as fast as you can."},
            {"Marquez un maximum de points en 2 minutes.", "Score as many points as you can in 2 minutes."},
            {"Sans gravité ni défaite : jouez à votre rythme.", "No gravity, no game over: play at your own pace."},
        }};

        app.fenetre.clear(COULEUR_FOND);
        fond.dessiner();
        DessinerTitre(TrU("Mode de jeu", "Game mode"), 80.f);
        liste.Dessiner();
        if (liste.Selection() < NB_MODES) {
            const Mode m = static_cast<Mode>(liste.Selection());
            const auto& [fr, en] = descriptions[static_cast<size_t>(m)];
            DessinerTexte(TrU(fr, en), 17, 455.f, GRIS);
            DessinerTexte(TexteRecord(m, classements.Premier(m)), 19, 490.f, JAUNE);
        }
        AfficherImage(aJour);
    }
    return std::nullopt;
}

void Menu::AfficherClassements(const Fond& fond, const Classements& classements) {
    int onglet = static_cast<int>(app.reglages.mode);
    constexpr std::array colonnes = {230.f, 370.f, 510.f, 660.f};

    bool aJour = false;
    while (app.fenetre.isOpen()) {
        while (const std::optional evenement = ProchainEvenement(aJour, fond.anime)) {
            if (app.GererEvenement(*evenement)) continue;
            if (evenement->is<sf::Event::KeyPressed>()) {
                switch (Appui(*evenement)) {
                    case Touche::Escape:
                    case Touche::Enter:
                    case Touche::Backspace: return;
                    case Touche::Left:  onglet = (onglet + NB_MODES - 1) % NB_MODES; break;
                    case Touche::Right: onglet = (onglet + 1) % NB_MODES; break;
                    default: break;
                }
            } else if (const auto* molette = evenement->getIf<sf::Event::MouseWheelScrolled>()) {
                if (molette->delta != 0.f) onglet = (onglet + (molette->delta > 0.f ? NB_MODES - 1 : 1)) % NB_MODES;
            } else if (const auto* clic = evenement->getIf<sf::Event::MouseButtonPressed>()) {
                // Clic sur un onglet pour le choisir, ailleurs pour revenir
                const sf::Vector2f p = app.fenetre.mapPixelToCoords(clic->position);
                if (std::abs(p.y - 100.f) < 20.f && std::abs(p.x - CENTRE_X) < 300.f)
                    onglet = std::clamp(static_cast<int>((p.x - (CENTRE_X - 300.f)) / 150.f), 0, NB_MODES - 1);
                else
                    return;
            }
        }
        if (!app.fenetre.isOpen()) break;

        const Mode m = static_cast<Mode>(onglet);
        app.fenetre.clear(COULEUR_FOND);
        fond.dessiner();
        DessinerTitre(TrU("Classements", "Leaderboards"), 45.f);

        for (int i = 0; i < NB_MODES; i++)
            DessinerTexte(NomMode(static_cast<Mode>(i)), 20, {CENTRE_X - 225.f + 150.f * static_cast<float>(i), 100.f},
                          i == onglet ? JAUNE : GRIS, 140.f);
        sf::RectangleShape souligne({110.f, 3.f});
        souligne.setOrigin({55.f, 0.f});
        souligne.setPosition({CENTRE_X - 225.f + 150.f * static_cast<float>(onglet), 116.f});
        souligne.setFillColor(JAUNE);
        app.fenetre.draw(souligne);

        const std::array<sf::String, 4> entetes = {Utf8("#"), m == Mode::Sprint ? TrU("Temps", "Time") : Utf8("Score"),
                                                   TrU("Lignes", "Lines"), Utf8("Date")};
        for (size_t c = 0; c < entetes.size(); c++) DessinerTexte(entetes[c], 16, {colonnes[c], 145.f}, GRIS, 150.f);

        const auto& table = classements.Table(m);
        if (table.empty())
            DessinerTexte(TrU("Aucune partie classée pour l'instant", "No ranked game yet"), 20, 300.f, GRIS);
        for (size_t i = 0; i < table.size(); i++) {
            const EntreeClassement& e = table[i];
            const float y = 178.f + 31.f * static_cast<float>(i);
            const sf::Color couleur = i == 0 ? JAUNE : sf::Color::White;
            DessinerTexte(Utf8(std::to_string(i + 1)), 19, {colonnes[0], y}, couleur, 60.f);
            DessinerTexte(Utf8(ValeurClassee(m, e)), 19, {colonnes[1], y}, couleur, 150.f);
            DessinerTexte(Utf8(FormaterNombre(e.lignes)), 19, {colonnes[2], y}, couleur, 120.f);
            DessinerTexte(Utf8(e.date.empty() ? "—" : e.date), 17, {colonnes[3], y}, GRIS, 180.f);
        }

        DessinerTexte(TrU("←→ changer de mode · Échap retour", "←→ switch mode · Esc back"), 15, 520.f, GRIS);
        AfficherImage(aJour);
    }
}

// ---------------------------------------------------------------------------------------------
// Options
// ---------------------------------------------------------------------------------------------

void Menu::EcranReglages(const Fond& fond, const sf::String& titre, std::span<const ElementReglage> elements) {
    Liste liste(app, 145.f, 42.f, 20, 700.f);
    const int retour = static_cast<int>(elements.size());

    bool aJour = false;
    const auto remplir = [&] {
        std::vector<sf::String> lignes;
        for (const ElementReglage& e : elements) lignes.push_back(e.libelle());
        lignes.push_back(TrU("Retour", "Back"));
        liste.Definir(std::move(lignes));
    };
    while (app.fenetre.isOpen()) {
        remplir();
        while (const std::optional evenement = ProchainEvenement(aJour, fond.anime || liste.EnAnimation())) {
            if (app.GererEvenement(*evenement)) continue;
            if (Echap(*evenement)) {
                app.SauverReglages();
                return;
            }
            const Liste::Resultat res = liste.Traiter(*evenement);
            const int ligne = res.activee >= 0 ? res.activee : liste.Selection();
            if (res.activee < 0 && res.delta == 0) continue;
            if (ligne == retour) {
                if (res.activee < 0) continue;
                app.SauverReglages();
                return;
            }
            // Valider une valeur l'augmente (et revient au début après la fin) ; ←→ la modifie
            elements[static_cast<size_t>(ligne)].modifier(res.activee >= 0 ? 1 : res.delta, res.activee >= 0);
        }
        if (!app.fenetre.isOpen()) break;

        remplir();

        app.fenetre.clear(COULEUR_FOND);
        fond.dessiner();
        DessinerTitre(titre, 70.f);
        liste.Dessiner();
        DessinerTexte(TrU("↑↓ choisir · ←→ ou molette modifier · Échap retour",
                          "↑↓ select · ←→ or wheel change · Esc back"),
                      15, 520.f, GRIS);
        AfficherImage(aJour);
    }
    app.SauverReglages();
}

void Menu::Options(const Fond& fond) {
    Reglages& r = app.reglages;
    auto booleen = [](bool& valeur) { return [&valeur](int, bool) { valeur = !valeur; }; };
    auto nombre = [](int& valeur, Bornes bornes) {
        return [&valeur, bornes](int delta, bool valide) { Ajuster(valeur, bornes, delta, valide); };
    };

    const std::vector<ElementReglage> jeu = {
        {[&] { return TrU("Délai avant répétition : ", "Delayed auto shift: ") + Duree(r.dasMs, "aucun", "none"); },
         nombre(r.dasMs, Reglages::BORNES_DAS)},
        {[&] { return TrU("Vitesse de répétition : ", "Auto repeat rate: ") + Duree(r.arrMs, "instantanée", "instant"); },
         nombre(r.arrMs, Reglages::BORNES_ARR)},
        {[&] { return TrU("Descente rapide : ", "Soft drop speed: ") + Duree(r.descenteDouceMs, "instantanée", "instant"); },
         nombre(r.descenteDouceMs, Reglages::BORNES_DESCENTE)},
        {[&] { return TrU("Délai de verrouillage : ", "Lock delay: ") + Duree(r.verrouillageMs, "aucun", "none"); },
         nombre(r.verrouillageMs, Reglages::BORNES_VERROUILLAGE)},
        {[&] { return TrU("Pièce fantôme : ", "Ghost piece: ") + OuiNon(r.fantome); }, booleen(r.fantome)},
        {[&] { return TrU("Rotation et garde anticipées (IRS/IHS) : ", "Initial rotation/hold (IRS/IHS): ") +
                      OuiNon(r.rotationAnticipee); },
         booleen(r.rotationAnticipee)},
    };

    const std::vector<ElementReglage> affichage = {
        {[&] { return TrU("Mouvements fluides : ", "Smooth movement: ") + OuiNon(r.mouvementsFluides); },
         booleen(r.mouvementsFluides)},
        {[&] { return TrU("Effets visuels : ", "Visual effects: ") + OuiNon(r.effets); }, booleen(r.effets)},
        {[&] { return TrU("Secousses du plateau : ", "Board shake: ") + OuiNon(r.secousses); }, booleen(r.secousses)},
        {[&] { return TrU("Synchro verticale : ", "V-sync: ") + OuiNon(r.synchroVerticale); },
         [&](int, bool) {
             r.synchroVerticale = !r.synchroVerticale;
             app.AppliquerSynchroVerticale();
         }},
        {[&] {
             return TrU("Images par seconde max : ", "Max frame rate: ") +
                    (r.limiteImages == 0 ? Utf8("auto") : Utf8(std::to_string(r.limiteImages)));
         },
         [&](int delta, bool valide) { r.limiteImages = LimiteVoisine(r.limiteImages, delta, valide); }},
        {[&] { return TrU("Plein écran : ", "Fullscreen: ") + OuiNon(r.pleinEcran); },
         [&](int, bool) {
             r.pleinEcran = !r.pleinEcran;
             app.AppliquerPleinEcran();
         }},
    };

    const std::vector<ElementReglage> accessibilite = {
        {[&] { return TrU("Palette pour daltoniens : ", "Colorblind palette: ") + OuiNon(r.daltonien); }, booleen(r.daltonien)},
        {[&] { return TrU("Motifs sur les pièces : ", "Piece patterns: ") + OuiNon(r.motifs); }, booleen(r.motifs)},
        {[&] { return TrU("Taille du texte des menus : ", "Menu text size: ") + Utf8(std::format("{} %", r.tailleTexte)); },
         nombre(r.tailleTexte, Reglages::BORNES_TAILLE_TEXTE)},
    };

    Liste liste(app, 175.f, 48.f, 22, 460.f, {true});
    bool aJour = false;
    const auto remplir = [&] {
        liste.Definir({TrU("Jeu", "Gameplay"), TrU("Affichage", "Display"), TrU("Accessibilité", "Accessibility"),
                       TrU("Langue : Français", "Language: English"), TrU("Valeurs par défaut", "Reset to defaults"),
                       TrU("Retour", "Back")});
    };
    while (app.fenetre.isOpen()) {
        remplir();
        while (const std::optional evenement = ProchainEvenement(aJour, fond.anime || liste.EnAnimation())) {
            if (app.GererEvenement(*evenement)) continue;
            if (Echap(*evenement)) {
                app.SauverReglages();
                return;
            }
            const Liste::Resultat res = liste.Traiter(*evenement);
            const bool langueModifiee = res.delta != 0 && liste.Selection() == 3;
            switch (langueModifiee ? 3 : res.activee) {
                case 0: EcranReglages(fond, TrU("Jeu", "Gameplay"), jeu); break;
                case 1: EcranReglages(fond, TrU("Affichage", "Display"), affichage); break;
                case 2: EcranReglages(fond, TrU("Accessibilité", "Accessibility"), accessibilite); break;
                case 3:
                    r.langue = r.langue == Langue::Francais ? Langue::Anglais : Langue::Francais;
                    DefinirLangue(r.langue);
                    app.SauverReglages();
                    break;
                case 4: {
                    const bool pleinEcranAvant = r.pleinEcran;
                    r.ReinitialiserOptions();
                    if (r.pleinEcran != pleinEcranAvant) app.AppliquerPleinEcran();
                    else app.AppliquerSynchroVerticale();
                    app.SauverReglages();
                    break;
                }
                case 5:
                    app.SauverReglages();
                    return;
                default: break;
            }
        }
        if (!app.fenetre.isOpen()) break;

        remplir();

        app.fenetre.clear(COULEUR_FOND);
        fond.dessiner();
        DessinerTitre(TrU("Options", "Options"), 80.f);
        liste.Dessiner();
        DessinerTexte(TrU("↑↓ choisir · Entrée valider · Échap retour", "↑↓ select · Enter confirm · Esc back"), 15, 520.f, GRIS);
        AfficherImage(aJour);
    }
    app.SauverReglages();
}

void Menu::Commandes(const Fond& fond) {
    Liste liste(app, 122.f, 33.f, 17, 820.f);
    Reglages& r = app.reglages;
    const int LIGNE_DEFAUT = NB_ACTIONS;
    const int LIGNE_RETOUR = NB_ACTIONS + 1;
    std::optional<Action> enAttente; // action qui attend une nouvelle touche ou un bouton

    auto terminerSaisie = [&] {
        enAttente.reset();
        traduireManette = true;
    };

    bool aJour = false;
    const auto remplir = [&] {
        auto liste2 = [](const std::array<int, TOUCHES_PAR_ACTION>& codes) {
            sf::String resultat = touches::Nom(codes[0]);
            if (codes[1] != AUCUNE_TOUCHE) resultat += " / " + touches::Nom(codes[1]);
            return resultat;
        };
        std::vector<sf::String> lignes;
        for (int a = 0; a < NB_ACTIONS; a++) {
            const Action action = static_cast<Action>(a);
            sf::String ligne = touches::NomAction(action) + TrU(" : ", ": ");
            if (enAttente == action)
                ligne += TrU("appuyez sur une touche ou un bouton…", "press a key or a button…");
            else
                ligne += liste2(r.Touches(action)) + TrU("    ·    manette : ", "    ·    gamepad: ") + liste2(r.Boutons(action));
            lignes.push_back(ligne);
        }
        lignes.push_back(TrU("Touches par défaut", "Reset to defaults"));
        lignes.push_back(TrU("Retour", "Back"));
        liste.Definir(std::move(lignes));
    };
    while (app.fenetre.isOpen()) {
        remplir();
        while (const std::optional evenement = ProchainEvenement(aJour, fond.anime || liste.EnAnimation())) {
            if (app.GererEvenement(*evenement)) continue;

            if (enAttente) {
                if (const auto* touche = evenement->getIf<sf::Event::KeyPressed>()) {
                    const bool attribuable = touches::Attribuable(touche->code);
                    if (touche->code != Touche::Escape && attribuable)
                        r.AssignerTouche(*enAttente, touches::CodeTouche(touche->code));
                    if (touche->code == Touche::Escape || attribuable) terminerSaisie();
                } else if (evenement->is<sf::Event::MouseButtonPressed>()) {
                    terminerSaisie();
                } else {
                    // Manette : premier bouton ou direction franche
                    for (const manette::Entree& entree : traducteur.Traduire(*evenement)) {
                        if (!entree.appui) continue;
                        r.AssignerBouton(*enAttente, entree.code);
                        terminerSaisie();
                        break;
                    }
                }
                continue;
            }

            if (Echap(*evenement)) {
                app.SauverReglages();
                return;
            }
            if (liste.Selection() < NB_ACTIONS) {
                const Action action = static_cast<Action>(liste.Selection());
                if (Appui(*evenement) == Touche::Backspace) {
                    r.EffacerTouches(action);
                    continue;
                }
                if (Appui(*evenement) == Touche::Delete) {
                    r.EffacerBoutons(action);
                    continue;
                }
            }

            const int activee = liste.Traiter(*evenement).activee;
            if (activee >= 0 && activee < NB_ACTIONS) {
                enAttente = static_cast<Action>(activee);
                traduireManette = false; // les boutons doivent arriver bruts pour être attribués
            } else if (activee == LIGNE_DEFAUT) {
                const Reglages defaut = touches::ParDefaut();
                r.touches = defaut.touches;
                r.manette = defaut.manette;
            } else if (activee == LIGNE_RETOUR) {
                app.SauverReglages();
                return;
            }
        }
        if (!app.fenetre.isOpen()) break;

        remplir();

        app.fenetre.clear(COULEUR_FOND);
        fond.dessiner();
        DessinerTitre(TrU("Commandes", "Controls"), 36.f);
        DessinerTexte(enAttente ? TrU("Échap ou clic : annuler", "Esc or click: cancel")
                                : TrU("Entrée : changer · Retour arrière : effacer le clavier · Suppr : effacer la manette · Échap : retour",
                                      "Enter: change · Backspace: clear keyboard · Delete: clear gamepad · Esc: back"),
                      14, 84.f, GRIS);
        liste.Dessiner();
        AfficherImage(aJour);
    }
    traduireManette = true;
    app.SauverReglages();
}

// ---------------------------------------------------------------------------------------------
// En partie
// ---------------------------------------------------------------------------------------------

bool Menu::ConfirmerSurFond(const Fond& fond, const std::string& question) {
    Liste liste(app, 290.f, 52.f, 24, 320.f, {true});

    bool aJour = false;
    const auto remplir = [&] {
        liste.Definir({TrU("Non", "No"), TrU("Oui", "Yes")});
    };
    while (app.fenetre.isOpen()) {
        remplir();
        while (const std::optional evenement = ProchainEvenement(aJour, fond.anime || liste.EnAnimation())) {
            if (app.GererEvenement(*evenement)) continue;
            if (Echap(*evenement)) return false;
            const int activee = liste.Traiter(*evenement).activee;
            if (activee >= 0) return activee == 1;
        }
        if (!app.fenetre.isOpen()) break;

        remplir();
        app.fenetre.clear(COULEUR_FOND);
        fond.dessiner();
        DessinerTexte(Utf8(question), 34, 200.f);
        liste.Dessiner();
        AfficherImage(aJour);
    }
    return false;
}

bool Menu::Confirmer(const sf::Texture& scene, const std::string& question) {
    PreparerFlou(scene);
    return ConfirmerSurFond({[&] { DessinerFondFlou(); }, false}, question);
}

bool Menu::Pause(const sf::Texture& scene) {
    PreparerFlou(scene);
    const Fond fond{[&] { DessinerFondFlou(); }, false};

    Liste liste(app, 250.f, 52.f, 24, 380.f, {true});

    bool aJour = false;
    const auto remplir = [&] {
        liste.Definir({TrU("Reprendre", "Resume"), TrU("Options", "Options"), TrU("Commandes", "Controls"),
                       TrU("Abandonner", "Quit game")});
    };
    while (app.fenetre.isOpen()) {
        remplir();
        while (const std::optional evenement = ProchainEvenement(aJour, liste.EnAnimation())) {
            if (app.GererEvenement(*evenement)) continue;
            const Touche touche = Appui(*evenement);
            if (touche == Touche::Escape ||
                (touche != Touche::Unknown && app.reglages.ActionDe(touches::CodeTouche(touche)) == Action::Pause))
                return false;

            switch (liste.Traiter(*evenement).activee) {
                case 0: return false;
                case 1: Options(fond); break;
                case 2: Commandes(fond); break;
                case 3:
                    if (ConfirmerSurFond(fond, Tr("Abandonner la partie ?", "Quit this game?"))) return true;
                    break;
                default: break;
            }
        }
        if (!app.fenetre.isOpen()) break;

        remplir();
        app.fenetre.clear(COULEUR_FOND);
        fond.dessiner();
        DessinerTitre(Utf8("Pause"), 160.f);
        liste.Dessiner();
        AfficherImage(aJour);
    }
    return false;
}

void Menu::CompteARebours(const sf::Texture& scene) {
    const float duree = 3.f * cst::COMPTE_A_REBOURS_S;
    sf::Clock horloge;

    while (app.fenetre.isOpen()) {
        const float t = horloge.getElapsedTime().asSeconds();
        if (t >= duree) return;

        while (const std::optional evenement = Lire()) app.GererEvenement(*evenement);
        if (!app.fenetre.isOpen()) return;

        const int chiffre = 3 - static_cast<int>(t / cst::COMPTE_A_REBOURS_S);
        const float progression = std::fmod(t, cst::COMPTE_A_REBOURS_S) / cst::COMPTE_A_REBOURS_S;

        const sf::FloatRect zone = app.ZoneVisible();
        sf::RectangleShape voile(zone.size);
        voile.setPosition(zone.position);
        voile.setFillColor(sf::Color(0, 0, 0, 110));

        app.fenetre.clear(COULEUR_FOND);
        DessinerDansZone(scene);
        app.fenetre.draw(voile);

        // Chaque chiffre apparaît en grand puis rétrécit en s'effaçant
        texte.setString(std::to_string(chiffre));
        texte.setFillColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(255.f * (1.f - std::pow(progression, 3.f)))));
        const float zoom = 1.f + 0.6f * std::pow(1.f - progression, 3.f);
        PlacerTexte(texte, 90, {CENTRE_X, CENTRE_Y}, app.Echelle(), zoom);
        app.fenetre.draw(texte);
        app.Afficher();
    }
}

Menu::Choix Menu::FinDePartie(const sf::Texture& scene, const ResumePartie& resume,
                              const std::function<void()>& revoir) {
    PreparerFlou(scene);
    const Mode m = resume.parametres.mode;

    // Entrées du menu : « Revoir la partie » seulement si le journal est disponible
    enum class Entree { Recommencer, Revoir, MenuPrincipal, Quitter };
    std::vector<Entree> entrees = {Entree::Recommencer};
    if (revoir) entrees.push_back(Entree::Revoir);
    entrees.insert(entrees.end(), {Entree::MenuPrincipal, Entree::Quitter});
    Liste liste(app, 366.f, 40.f, 20, 380.f, {true});

    bool aJour = false;
    const auto remplir = [&] {
        std::vector<sf::String> libelles;
        for (Entree e : entrees) {
            switch (e) {
                case Entree::Recommencer:   libelles.push_back(TrU("Recommencer", "Play again")); break;
                case Entree::Revoir:        libelles.push_back(TrU("Revoir la partie", "Watch replay")); break;
                case Entree::MenuPrincipal: libelles.push_back(TrU("Menu principal", "Main menu")); break;
                case Entree::Quitter:       libelles.push_back(TrU("Quitter", "Quit")); break;
            }
        }
        liste.Definir(std::move(libelles));
    };
    while (app.fenetre.isOpen()) {
        remplir();
        while (const std::optional evenement = ProchainEvenement(aJour, liste.EnAnimation())) {
            if (app.GererEvenement(*evenement)) continue;
            if (Echap(*evenement)) return Choix::MenuPrincipal;
            const int activee = liste.Traiter(*evenement).activee;
            if (activee < 0) continue;
            switch (entrees[static_cast<size_t>(activee)]) {
                case Entree::Recommencer: return Choix::Jouer;
                case Entree::Revoir:
                    revoir();
                    aJour = false;
                    break;
                case Entree::MenuPrincipal: return Choix::MenuPrincipal;
                case Entree::Quitter: return Choix::Quitter;
            }
        }
        if (!app.fenetre.isOpen()) break;

        remplir();

        sf::String titre = TrU("Partie terminée", "Game over");
        if (m == Mode::Sprint && resume.objectifAtteint) titre = TrU("Sprint terminé !", "Sprint complete!");
        if (m == Mode::Ultra && resume.objectifAtteint) titre = TrU("Temps écoulé !", "Time's up!");
        if (m == Mode::Zen) titre = TrU("Session zen terminée", "Zen session over");

        // Résultat principal : temps en Sprint réussi, score sinon
        sf::String principal;
        if (m == Mode::Sprint)
            principal = resume.objectifAtteint ? Utf8(FormaterTemps(resume.temps))
                                               : TrU("Objectif non atteint", "Not finished");
        else
            principal = Utf8(FormaterNombre(resume.score)) + Utf8(" pts");

        sf::String classement;
        sf::Color couleurClassement = sf::Color::White;
        if (resume.rang == 0) {
            classement = TrU("Nouveau record !", "New record!");
            couleurClassement = JAUNE;
        } else if (resume.rang > 0) {
            classement = Utf8(LangueActuelle() == Langue::Anglais ? std::format("Ranked #{}", resume.rang + 1)
                                                                   : std::format("Classé {}e", resume.rang + 1));
        } else {
            classement = TexteRecord(m, resume.premier ? &*resume.premier : nullptr);
            couleurClassement = GRIS;
        }

        const Statistiques& s = resume.stats;
        const float pps = resume.temps > 0.f ? static_cast<float>(s.pieces) / resume.temps : 0.f;
        const std::array<sf::String, 8> statistiques = {
            TrU("Lignes : ", "Lines: ") + Utf8(FormaterNombre(resume.lignes)),
            m == Mode::Marathon ? TrU("Niveau : ", "Level: ") + Utf8(std::to_string(resume.niveau))
                                : TrU("Mode : ", "Mode: ") + NomMode(m),
            TrU("Durée : ", "Time: ") + Utf8(FormaterTemps(resume.temps)),
            TrU("Pièces : ", "Pieces: ") + Utf8(FormaterNombre(s.pieces)),
            TrU("Pièces par seconde : ", "Pieces per second: ") + Utf8(Decimal(pps)),
            TrU("Simples : ", "Singles: ") + Utf8(std::to_string(s.lignesParType[0])) + TrU(" · Doubles : ", " · Doubles: ") +
                Utf8(std::to_string(s.lignesParType[1])),
            TrU("Triples : ", "Triples: ") + Utf8(std::to_string(s.lignesParType[2])) + TrU(" · Tetris : ", " · Tetris: ") +
                Utf8(std::to_string(s.lignesParType[3])),
            TrU("Meilleur combo : ×", "Best combo: ×") + Utf8(std::to_string(s.comboMax)),
        };

        app.fenetre.clear(COULEUR_FOND);
        DessinerFondFlou();
        DessinerTitre(titre, 44.f);
        DessinerTexte(principal, 34, 108.f, resume.rang == 0 ? JAUNE : sf::Color::White);
        DessinerTexte(classement, 20, 146.f, couleurClassement);
        for (size_t i = 0; i < statistiques.size(); i++) {
            const float x = i < 4 ? CENTRE_X - 170.f : CENTRE_X + 170.f;
            const float y = 190.f + 32.f * static_cast<float>(i % 4);
            DessinerTexte(statistiques[i], 17, {x, y}, sf::Color(225, 225, 225), 320.f);
        }
        liste.Dessiner();
        AfficherImage(aJour);
    }
    return Choix::Quitter;
}
