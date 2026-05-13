#include "bloc.h"
#include "menu.h"
#ifndef SFML_STATIC
#define SFML_STATIC
#endif
#include <SFML/Graphics.hpp>
#include <cmath>
#include <iostream>

// Helper

void SetText(sf::Text& text, sf::Font& font, int x, int y, int size = 20) {
    text.setFont(font);
    text.setCharacterSize(size);
    text.setPosition(sf::Vector2f(x, y));
    text.setFillColor(sf::Color::White);
    text.setStyle(sf::Text::Bold);
}

void CentrerTexte(sf::Text& text, float cx, float cy) {
    sf::FloatRect b = text.getLocalBounds();
    text.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
    text.setPosition(cx, cy);
}

// Combo graphics

void DrawCombo(sf::RenderWindow& window, sf::Text& textCombo, bloc& Monbloc, float time) {
    int comboVal = Monbloc.AfficherCombo();
    if (comboVal <= 0) return;

    float angleOscillation = std::sin(time * 4.f) * 5.f;

    textCombo.setString("COMBO X" + std::to_string(comboVal));
    textCombo.setOutlineThickness(2.f);
    textCombo.setOutlineColor(sf::Color::Black);
    textCombo.setRotation(angleOscillation);
    float scale = 1.f + std::sin(time * 10.f) * 0.1f;
    textCombo.setScale(scale, scale);

    sf::Color color;
    if      (comboVal >= 8) color = sf::Color(255,   0, 255);
    else if (comboVal >= 5) color = sf::Color(255,  50,  50);
    else if (comboVal >= 3) color = sf::Color(255, 165,   0);
    else                    color = sf::Color::Cyan;
    textCombo.setFillColor(color);

    sf::FloatRect cb = textCombo.getLocalBounds();
    textCombo.setOrigin(cb.width / 2.f, cb.height / 2.f);
    textCombo.setPosition(450.f, 150.f);
    window.draw(textCombo);

    // Barre de temps (pour combo)
    float ratio = std::max(0.f, std::min(1.f,
        Monbloc.TempsRestantCombo() / Monbloc.GetComboTimeLimit()));

    const float barW     = 140.f;
    const float barH     = 8.f;
    const float rad      = angleOscillation * 3.14159f / 180.f;
    const sf::Vector2f center(450.f, 178.f);

    // Fond
    sf::RectangleShape fond(sf::Vector2f(barW, barH));
    fond.setOrigin(barW / 2.f, barH / 2.f);
    fond.setPosition(center);
    fond.setRotation(angleOscillation);
    fond.setFillColor(sf::Color(0, 0, 0, 150));
    fond.setOutlineThickness(1.5f);
    fond.setOutlineColor(sf::Color(255, 255, 255, 80));
    window.draw(fond);

    if (ratio > 0.01f) {
        // Point de départ du bord gauche
        float ox = (barW / 2.f) * std::cos(rad);
        float oy = (barW / 2.f) * std::sin(rad);
        sf::Vector2f left(center.x - ox, center.y - oy);

        // Couleur
        sf::Color fillCol;
        if      (ratio > 0.5f) fillCol = sf::Color(  0, 255, 150);
        else if (ratio > 0.2f) fillCol = sf::Color(255, 200,   0);
        else                   fillCol = sf::Color(255,  50,  50);

        // Remplissage
        sf::RectangleShape fill(sf::Vector2f(barW * ratio, barH));
        fill.setOrigin(0.f, barH / 2.f);
        fill.setPosition(left);
        fill.setRotation(angleOscillation);
        fill.setFillColor(fillCol);
        window.draw(fill);

        // Brillance
        sf::RectangleShape shine(sf::Vector2f(barW * ratio, barH / 2.f));
        shine.setOrigin(0.f, barH / 4.f);
        shine.setPosition(left);
        shine.setRotation(angleOscillation);
        shine.setFillColor(sf::Color(255, 255, 255, 50));
        window.draw(shine);
    }
}

// Rendu du jeu (grille, pièces, textes)

void DrawGame(sf::RenderWindow& window, bloc& Monbloc, sf::Sprite& fond,
              sf::Text& textScore, sf::Text& textNiveau, sf::Text& textLignes,
              sf::Text& textCombo, float time)
{
    window.clear(sf::Color(15, 15, 15));
    window.draw(fond);

    // Ligne limite rouge
    sf::RectangleShape limiteLine(sf::Vector2f(180.f, 2.f));
    limiteLine.setFillColor(sf::Color(255, 0, 0, 150));
    limiteLine.setPosition(360.f, 208.f);
    window.draw(limiteLine);

    // Grille + pièces UI
    Monbloc.DessinerLeTableau();
    Monbloc.next();
    Monbloc.Saved();
    Monbloc.VisualiserBloc();

    // Textes stats 
    textScore.setString(Monbloc.AfficherScore());
    CentrerTexte(textScore, (650.f + 827.f) / 2.f, 282.f);

    textNiveau.setString(Monbloc.AfficherNiveau());
    CentrerTexte(textNiveau, (66.f + 246.f) / 2.f, 470.f);

    textLignes.setString(Monbloc.AfficherLigneDetruite());
    CentrerTexte(textLignes, (66.f + 246.f) / 2.f, 345.f);

    window.draw(textScore);
    window.draw(textNiveau);
    window.draw(textLignes);

    // Combo
    Monbloc.UpdateCombo();
    DrawCombo(window, textCombo, Monbloc, time);

    window.display();
}

int main() {
    sf::RenderWindow window(sf::VideoMode(900, 540), "Tetris game");
    window.setVerticalSyncEnabled(true);

    sf::Font font;
    if (!font.loadFromFile("asset/arial.ttf")) return EXIT_FAILURE;

    sf::Texture TextTruc, FondPrincipal;
    if (!TextTruc.loadFromFile("asset/tiles.png"))        return EXIT_FAILURE;
    if (!FondPrincipal.loadFromFile("asset/FondPrincipal.png")) return EXIT_FAILURE;

    sf::Sprite FondP(FondPrincipal);

    sf::Text textScore, textNiveau, textLignes, textCombo, textNextPiece;
    SetText(textScore,     font, 0, 0);
    SetText(textNiveau,    font, 0, 0);
    SetText(textLignes,    font, 0, 0);
    SetText(textCombo,     font, 0, 0, 25);
    SetText(textNextPiece, font, 582, 70, 15);
    textNextPiece.setString("Prochaine piece : ");

    menu Menu(window, font);
    int MenuOptions = Menu.MenuJeu();

    // Boucle principale
    while (window.isOpen()) {
        bloc Monbloc(TextTruc, &window, 360, 136);
        Monbloc.BlocAleatoire();
        Monbloc.CouleurAleatoire();
        Monbloc.RegenererBloc();

        if (MenuOptions == 0) { window.close(); break; }

        sf::Clock gravityClock;
        sf::Clock frameClock;

        // Boucle de jeu
        while (!Monbloc.Perdu() && window.isOpen()) {

            // Gravité
            if (gravityClock.getElapsedTime().asMilliseconds() > Monbloc.VitesseBloc()) {
                if (!Monbloc.checkmove(0, 1)) {
                    int lignes = Monbloc.ClearLines();
                    if (lignes > 0) Monbloc.ScoreAdd("Ligne", lignes);
                    Monbloc.ResetBloc();
                } else {
                    Monbloc.mouvement("down");
                }
                gravityClock.restart();
            }

            // Événements
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) { window.close(); break; }

                if (event.type == sf::Event::KeyPressed) {
                    switch (event.key.code) {
                        case sf::Keyboard::Enter:  Monbloc.RotationBloc();             break;
                        case sf::Keyboard::Right:  Monbloc.mouvement("right");         break;
                        case sf::Keyboard::Left:   Monbloc.mouvement("left");          break;
                        case sf::Keyboard::Space:  Monbloc.AtterirEnBas();             break;
                        case sf::Keyboard::RShift: Monbloc.ChangerBloc();              break;
                        case sf::Keyboard::Down:
                            if (!Monbloc.DetectionBlocEmpile()) {
                                Monbloc.mouvement("down");
                                Monbloc.ScoreAdd("DescenteRapide", 0);
                                gravityClock.restart();
                            }
                            break;
                        default: break;
                    }
                }
            }

            Monbloc.ChangementNiveau();
            DrawGame(window, Monbloc, FondP,
                     textScore, textNiveau, textLignes, textCombo,
                     frameClock.getElapsedTime().asSeconds());
        }

        if (!window.isOpen()) break;

        sf::Texture textureFond;
        textureFond.create(900, 540);
        textureFond.update(window);

        int choix = Menu.MenuPerdu(Monbloc.AfficherScore(), textureFond);
        if (choix == 1) window.close();
        // choix == 0 → on reboucle et recrée un bloc
    }

    return 0;
}