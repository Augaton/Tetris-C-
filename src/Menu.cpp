#include "Menu.h"

#include "Constantes.h"
#include "Texte.h"
#include "Touches.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

// Flou gaussien séparable : une passe horizontale puis une verticale,
// 11 lectures par pixel par passe au lieu de 121 (ou 625) pour un flou 2D direct
const char* flou_frag = R"(
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
const float LARGEUR_FLOU = static_cast<float>(cst::FENETRE_LARGEUR);

// Ne recrée la texture que si sa taille change
bool Dimensionner(sf::RenderTexture& texture, sf::Vector2u taille) {
    if (texture.getSize() == taille) return true;
    if (!texture.create(taille.x, taille.y)) return false;
    texture.setSmooth(true);
    return true;
}

const float CENTRE_X = static_cast<float>(cst::FENETRE_LARGEUR) / 2.f;
const float CENTRE_Y = static_cast<float>(cst::FENETRE_HAUTEUR) / 2.f;
const sf::Color JAUNE(255, 204, 0);
const sf::Color GRIS(170, 170, 170);

// Liste verticale d'entrées : flèches haut/bas pour choisir, Entrée/Espace/clic pour valider,
// flèches gauche/droite, clic droit ou molette pour modifier une valeur.
class Liste {
public:
    struct Resultat {
        int activee = -1; // entrée validée
        int delta = 0;    // modification demandée sur l'entrée sélectionnée
    };

    Liste(Application& app, float haut, float espacement, unsigned taille, float largeur = 560.f)
        : app(app), haut(haut), espacement(espacement), taille(taille), largeur(largeur) {}

    void Definir(std::vector<sf::String> nouvelles) { lignes = std::move(nouvelles); }
    int Selection() const { return selection; }

    Resultat Traiter(const sf::Event& e) {
        const int n = static_cast<int>(lignes.size());
        if (n == 0) return {};

        switch (e.type) {
            case sf::Event::KeyPressed:
                switch (e.key.code) {
                    case sf::Keyboard::Up:    selection = (selection + n - 1) % n; break;
                    case sf::Keyboard::Down:  selection = (selection + 1) % n;     break;
                    case sf::Keyboard::Enter:
                    case sf::Keyboard::Space: return {selection, 0};
                    case sf::Keyboard::Left:  return {-1, -1};
                    case sf::Keyboard::Right: return {-1, 1};
                    default: break;
                }
                break;
            case sf::Event::MouseMoved: {
                const int ligne = LigneSous(e.mouseMove.x, e.mouseMove.y);
                if (ligne >= 0) selection = ligne;
                break;
            }
            case sf::Event::MouseButtonPressed: {
                const int ligne = LigneSous(e.mouseButton.x, e.mouseButton.y);
                if (ligne < 0) break;
                selection = ligne;
                if (e.mouseButton.button == sf::Mouse::Left) return {ligne, 0};
                if (e.mouseButton.button == sf::Mouse::Right) return {-1, -1};
                break;
            }
            case sf::Event::MouseWheelScrolled: {
                const int ligne = LigneSous(e.mouseWheelScroll.x, e.mouseWheelScroll.y);
                if (ligne < 0 || e.mouseWheelScroll.delta == 0.f) break;
                selection = ligne;
                return {-1, e.mouseWheelScroll.delta > 0.f ? 1 : -1};
            }
            default:
                break;
        }
        return {};
    }

    void Dessiner(sf::Text& texte) {
        const float echelle = app.Echelle();
        for (size_t i = 0; i < lignes.size(); i++) {
            const bool choisie = static_cast<int>(i) == selection;
            texte.setString(lignes[i]);
            texte.setFillColor(choisie ? JAUNE : sf::Color::White);
            PlacerTexte(texte, taille, Centre(static_cast<int>(i)), echelle, choisie ? 1.08f : 1.f);
            const float largeurTexte = texte.getGlobalBounds().width;
            if (largeurTexte > largeur)
                PlacerTexte(texte, taille, Centre(static_cast<int>(i)), echelle, (choisie ? 1.08f : 1.f) * largeur / largeurTexte);
            app.fenetre.draw(texte);
        }
    }

private:
    Application& app;
    std::vector<sf::String> lignes;
    float haut, espacement;
    unsigned taille;
    float largeur;
    int selection = 0;

    sf::Vector2f Centre(int i) const { return {CENTRE_X, haut + espacement * static_cast<float>(i)}; }

    int LigneSous(int px, int py) const {
        const sf::Vector2f p = app.fenetre.mapPixelToCoords({px, py});
        if (std::abs(p.x - CENTRE_X) > largeur / 2.f) return -1;
        for (size_t i = 0; i < lignes.size(); i++)
            if (std::abs(p.y - Centre(static_cast<int>(i)).y) <= espacement / 2.f) return static_cast<int>(i);
        return -1;
    }
};

bool Echap(const sf::Event& e) {
    return e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Escape;
}

void Ajuster(int& valeur, const Bornes& bornes, int delta, bool boucler) {
    int nouvelle = valeur + delta * bornes.pas;
    if (boucler && nouvelle > bornes.max) nouvelle = bornes.min;
    valeur = bornes.Limiter(nouvelle);
}

sf::String Duree(int ms, const char* siZero) {
    return ms == 0 ? Utf8(siZero) : Utf8(std::to_string(ms) + " ms");
}

sf::String OuiNon(bool valeur) {
    return Utf8(valeur ? "Oui" : "Non");
}

} // namespace

Menu::Menu(Application& app) : app(app) {
    texte.setFont(app.police);
    texte.setStyle(sf::Text::Bold);

    // Sans shader (GPU non compatible) on affiche le fond sans flou au lieu de planter
    shaderOk = sf::Shader::isAvailable() && blurShader.loadFromMemory(flou_frag, sf::Shader::Fragment);
    if (!shaderOk) std::cerr << "Shader de flou indisponible\n";
}

void Menu::DessinerTexte(const sf::String& chaine, unsigned taille, float y, sf::Color couleur) {
    texte.setString(chaine);
    texte.setFillColor(couleur);
    PlacerTexteBorne(texte, taille, {CENTRE_X, y}, app.Echelle(), app.ZoneVisible().width - 20.f);
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
    reduit.setScale(static_cast<float>(taille.x) / static_cast<float>(source.x),
                    static_cast<float>(taille.y) / static_cast<float>(source.y));
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

// Les captures couvrent toute la fenêtre : on les étire sur la zone visible actuelle
void Menu::DessinerDansZone(const sf::Texture& texture) {
    if (texture.getSize().x == 0 || texture.getSize().y == 0) return;
    const sf::FloatRect zone = app.ZoneVisible();
    sf::Sprite sprite(texture);
    sprite.setPosition(zone.left, zone.top);
    sprite.setScale(zone.width / static_cast<float>(texture.getSize().x),
                    zone.height / static_cast<float>(texture.getSize().y));
    app.fenetre.draw(sprite);
}

Menu::Choix Menu::Principal(long long meilleurScore) {
    Liste liste(app, 262.f, 58.f, 30);
    liste.Definir({Utf8("Jouer !"), Utf8("Options"), Utf8("Commandes"), Utf8("Quitter")});

    // Fond qui défile vers le bas, répété pour couvrir toute la zone visible
    sf::Sprite spriteFond(app.fondMenu);
    sf::Clock horlogeFond;
    float decalage = 0.f;
    const DessinFond fond = [&] {
        const sf::FloatRect zone = app.ZoneVisible();
        const sf::Vector2f tailleTexture(app.fondMenu.getSize());
        const float echelleFond = std::max(1.f, zone.width / tailleTexture.x);
        const float hauteur = tailleTexture.y * echelleFond;

        decalage = std::fmod(decalage + 25.f * horlogeFond.restart().asSeconds(), hauteur);
        spriteFond.setScale(echelleFond, echelleFond);
        const float x = CENTRE_X - tailleTexture.x * echelleFond / 2.f;
        for (float y = zone.top + decalage - hauteur; y < zone.top + zone.height; y += hauteur) {
            spriteFond.setPosition(x, y);
            app.fenetre.draw(spriteFond);
        }
    };

    sf::Sprite spriteLogo(app.logo);
    const sf::FloatRect b = spriteLogo.getLocalBounds();
    spriteLogo.setOrigin(b.width / 2.f, b.height / 2.f);
    spriteLogo.setPosition(CENTRE_X, 100.f);

    while (app.fenetre.isOpen()) {
        sf::Event evenement;
        while (app.fenetre.pollEvent(evenement)) {
            if (app.GererEvenement(evenement)) continue;
            switch (liste.Traiter(evenement).activee) {
                case 0: return Choix::Jouer;
                case 1: Options(fond); break;
                case 2: Commandes(fond); break;
                case 3: return Choix::Quitter;
                default: break;
            }
        }
        if (!app.fenetre.isOpen()) break;

        app.fenetre.clear(COULEUR_FOND);
        fond();
        app.fenetre.draw(spriteLogo);
        liste.Dessiner(texte);
        if (meilleurScore > 0) DessinerTexte(Utf8("Meilleur score : " + std::to_string(meilleurScore)), 20, 490.f);
        DessinerTexte(Utf8("↑↓ choisir · Entrée valider · F11 plein écran"), 14, 525.f, GRIS);
        app.Afficher();
    }
    return Choix::Quitter;
}

void Menu::Options(const DessinFond& fond) {
    Liste liste(app, 122.f, 38.f, 22, 640.f);
    Reglages& r = app.reglages;

    while (app.fenetre.isOpen()) {
        sf::Event evenement;
        while (app.fenetre.pollEvent(evenement)) {
            if (app.GererEvenement(evenement)) continue;
            if (Echap(evenement)) {
                app.SauverReglages();
                return;
            }

            const Liste::Resultat res = liste.Traiter(evenement);
            if (res.activee < 0 && res.delta == 0) continue;
            // Valider une valeur numérique l'augmente et revient au minimum après le maximum
            const int delta = res.activee >= 0 ? 1 : res.delta;
            const bool boucler = res.activee >= 0;

            switch (liste.Selection()) {
                case 0: Ajuster(r.dasMs, Reglages::BORNES_DAS, delta, boucler); break;
                case 1: Ajuster(r.arrMs, Reglages::BORNES_ARR, delta, boucler); break;
                case 2: Ajuster(r.descenteDouceMs, Reglages::BORNES_DESCENTE, delta, boucler); break;
                case 3: Ajuster(r.verrouillageMs, Reglages::BORNES_VERROUILLAGE, delta, boucler); break;
                case 4: r.fantome = !r.fantome; break;
                case 5: r.effets = !r.effets; break;
                case 6: r.secousses = !r.secousses; break;
                case 7:
                    r.pleinEcran = !r.pleinEcran;
                    app.AppliquerPleinEcran();
                    break;
                case 8:
                    if (res.activee < 0) break;
                    {
                        const bool pleinEcranAvant = r.pleinEcran;
                        r.ReinitialiserOptions();
                        if (r.pleinEcran != pleinEcranAvant) app.AppliquerPleinEcran();
                    }
                    break;
                case 9:
                    if (res.activee < 0) break;
                    app.SauverReglages();
                    return;
                default: break;
            }
        }
        if (!app.fenetre.isOpen()) break;

        liste.Definir({
            Utf8("Délai avant répétition : ") + Duree(r.dasMs, "aucun"),
            Utf8("Vitesse de répétition : ") + Duree(r.arrMs, "instantanée"),
            Utf8("Descente rapide : ") + Duree(r.descenteDouceMs, "instantanée"),
            Utf8("Délai de verrouillage : ") + Duree(r.verrouillageMs, "aucun"),
            Utf8("Pièce fantôme : ") + OuiNon(r.fantome),
            Utf8("Effets visuels : ") + OuiNon(r.effets),
            Utf8("Secousses du plateau : ") + OuiNon(r.secousses),
            Utf8("Plein écran : ") + OuiNon(r.pleinEcran),
            Utf8("Valeurs par défaut"),
            Utf8("Retour"),
        });

        app.fenetre.clear(COULEUR_FOND);
        fond();
        DessinerTexte(Utf8("Options"), 40, 60.f);
        liste.Dessiner(texte);
        DessinerTexte(Utf8("↑↓ choisir · ←→ ou molette modifier · Échap retour"), 15, 520.f, GRIS);
        app.Afficher();
    }
    app.SauverReglages();
}

void Menu::Commandes(const DessinFond& fond) {
    Liste liste(app, 108.f, 35.f, 21, 640.f);
    Reglages& r = app.reglages;
    const int LIGNE_DEFAUT = NB_ACTIONS;
    const int LIGNE_RETOUR = NB_ACTIONS + 1;
    std::optional<Action> enAttente; // action qui attend une nouvelle touche

    while (app.fenetre.isOpen()) {
        sf::Event evenement;
        while (app.fenetre.pollEvent(evenement)) {
            if (app.GererEvenement(evenement)) continue;

            if (enAttente) {
                if (evenement.type == sf::Event::KeyPressed) {
                    if (evenement.key.code != sf::Keyboard::Escape && touches::Attribuable(evenement.key.code))
                        r.AssignerTouche(*enAttente, evenement.key.code);
                    if (evenement.key.code == sf::Keyboard::Escape || touches::Attribuable(evenement.key.code))
                        enAttente.reset();
                } else if (evenement.type == sf::Event::MouseButtonPressed) {
                    enAttente.reset();
                }
                continue;
            }

            if (Echap(evenement)) {
                app.SauverReglages();
                return;
            }
            if (evenement.type == sf::Event::KeyPressed && liste.Selection() < NB_ACTIONS &&
                (evenement.key.code == sf::Keyboard::Backspace || evenement.key.code == sf::Keyboard::Delete)) {
                r.EffacerTouches(static_cast<Action>(liste.Selection()));
                continue;
            }

            const int activee = liste.Traiter(evenement).activee;
            if (activee >= 0 && activee < NB_ACTIONS) {
                enAttente = static_cast<Action>(activee);
            } else if (activee == LIGNE_DEFAUT) {
                r.touches = touches::ParDefaut().touches;
            } else if (activee == LIGNE_RETOUR) {
                app.SauverReglages();
                return;
            }
        }
        if (!app.fenetre.isOpen()) break;

        std::vector<sf::String> lignes;
        for (int a = 0; a < NB_ACTIONS; a++) {
            const Action action = static_cast<Action>(a);
            sf::String ligne = touches::NomAction(action) + " : ";
            if (enAttente == action) {
                ligne += Utf8("appuyez sur une touche…");
            } else {
                const auto& t = r.Touches(action);
                ligne += touches::Nom(t[0]);
                if (t[1] != AUCUNE_TOUCHE) ligne += " / " + touches::Nom(t[1]);
            }
            lignes.push_back(ligne);
        }
        lignes.push_back(Utf8("Touches par défaut"));
        lignes.push_back(Utf8("Retour"));
        liste.Definir(std::move(lignes));

        app.fenetre.clear(COULEUR_FOND);
        fond();
        DessinerTexte(Utf8("Commandes"), 36, 40.f);
        DessinerTexte(enAttente ? Utf8("Échap ou clic : annuler")
                                : Utf8("Entrée : changer · Retour arrière : effacer · Échap : retour"),
                      15, 78.f, GRIS);
        liste.Dessiner(texte);
        app.Afficher();
    }
    app.SauverReglages();
}

bool Menu::ConfirmerSurFond(const DessinFond& fond, const std::string& question) {
    Liste liste(app, 290.f, 55.f, 28);
    liste.Definir({Utf8("Non"), Utf8("Oui")});

    while (app.fenetre.isOpen()) {
        sf::Event evenement;
        while (app.fenetre.pollEvent(evenement)) {
            if (app.GererEvenement(evenement)) continue;
            if (Echap(evenement)) return false;
            const int activee = liste.Traiter(evenement).activee;
            if (activee >= 0) return activee == 1;
        }
        if (!app.fenetre.isOpen()) break;

        app.fenetre.clear(COULEUR_FOND);
        fond();
        DessinerTexte(Utf8(question), 34, 200.f);
        liste.Dessiner(texte);
        app.Afficher();
    }
    return false;
}

bool Menu::Confirmer(const sf::Texture& scene, const std::string& question) {
    PreparerFlou(scene);
    return ConfirmerSurFond([&] { DessinerDansZone(fondFlou.getTexture()); }, question);
}

bool Menu::Pause(const sf::Texture& scene) {
    PreparerFlou(scene);
    const DessinFond fond = [&] { DessinerDansZone(fondFlou.getTexture()); };

    Liste liste(app, 250.f, 55.f, 28);
    liste.Definir({Utf8("Reprendre"), Utf8("Options"), Utf8("Commandes"), Utf8("Abandonner")});

    while (app.fenetre.isOpen()) {
        sf::Event evenement;
        while (app.fenetre.pollEvent(evenement)) {
            if (app.GererEvenement(evenement)) continue;
            if (evenement.type == sf::Event::KeyPressed &&
                (evenement.key.code == sf::Keyboard::Escape || app.reglages.ActionDe(evenement.key.code) == Action::Pause))
                return false;

            switch (liste.Traiter(evenement).activee) {
                case 0: return false;
                case 1: Options(fond); break;
                case 2: Commandes(fond); break;
                case 3:
                    if (ConfirmerSurFond(fond, "Abandonner la partie ?")) return true;
                    break;
                default: break;
            }
        }
        if (!app.fenetre.isOpen()) break;

        app.fenetre.clear(COULEUR_FOND);
        fond();
        DessinerTexte(Utf8("Pause"), 45, 160.f);
        liste.Dessiner(texte);
        app.Afficher();
    }
    return false;
}

void Menu::CompteARebours(const sf::Texture& scene) {
    const float duree = 3.f * cst::COMPTE_A_REBOURS_S;
    sf::Clock horloge;

    while (app.fenetre.isOpen()) {
        const float t = horloge.getElapsedTime().asSeconds();
        if (t >= duree) return;

        sf::Event evenement;
        while (app.fenetre.pollEvent(evenement)) app.GererEvenement(evenement);
        if (!app.fenetre.isOpen()) return;

        const int chiffre = 3 - static_cast<int>(t / cst::COMPTE_A_REBOURS_S);
        const float progression = std::fmod(t, cst::COMPTE_A_REBOURS_S) / cst::COMPTE_A_REBOURS_S;

        const sf::FloatRect zone = app.ZoneVisible();
        sf::RectangleShape voile({zone.width, zone.height});
        voile.setPosition(zone.left, zone.top);
        voile.setFillColor(sf::Color(0, 0, 0, 110));

        app.fenetre.clear(COULEUR_FOND);
        DessinerDansZone(scene);
        app.fenetre.draw(voile);

        // Chaque chiffre apparaît en grand puis rétrécit en s'effaçant
        texte.setString(std::to_string(chiffre));
        texte.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(255.f * (1.f - std::pow(progression, 3.f)))));
        const float zoom = 1.f + 0.6f * std::pow(1.f - progression, 3.f);
        PlacerTexte(texte, 90, {CENTRE_X, CENTRE_Y}, app.Echelle(), zoom);
        app.fenetre.draw(texte);
        app.Afficher();
    }
}

Menu::Choix Menu::Perdu(const sf::Texture& scene, long long score, long long meilleurScore, bool nouveauRecord) {
    PreparerFlou(scene);

    Liste liste(app, 255.f, 55.f, 32);
    liste.Definir({Utf8("Recommencer"), Utf8("Menu principal"), Utf8("Quitter")});

    while (app.fenetre.isOpen()) {
        sf::Event evenement;
        while (app.fenetre.pollEvent(evenement)) {
            if (app.GererEvenement(evenement)) continue;
            if (Echap(evenement)) return Choix::MenuPrincipal;
            switch (liste.Traiter(evenement).activee) {
                case 0: return Choix::Jouer;
                case 1: return Choix::MenuPrincipal;
                case 2: return Choix::Quitter;
                default: break;
            }
        }
        if (!app.fenetre.isOpen()) break;

        app.fenetre.clear(COULEUR_FOND);
        DessinerDansZone(fondFlou.getTexture());
        DessinerTexte(Utf8("Partie terminée"), 50, 160.f);
        liste.Dessiner(texte);
        DessinerTexte(Utf8("Votre score : " + std::to_string(score)), 30, 440.f);
        if (nouveauRecord)
            DessinerTexte(Utf8("Nouveau record !"), 24, 485.f, JAUNE);
        else
            DessinerTexte(Utf8("Meilleur score : " + std::to_string(meilleurScore)), 24, 485.f);
        app.Afficher();
    }
    return Choix::Quitter;
}
