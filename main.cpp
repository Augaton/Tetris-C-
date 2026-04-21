#include "bloc.h"
#include "menu.h"
#ifndef SFML_STATIC
#define SFML_STATIC
#endif
#include <SFML/Graphics.hpp>
#include <cmath>
#include <iostream>
#include <codecvt>






bloc* MonblocCopy;


void SetText(sf::Text &Text, sf::Font &font, int posX, int posY){
    Text.setFont(font);
    Text.setCharacterSize(20);
    Text.setPosition(sf::Vector2f(posX,posY));
    Text.setFillColor(sf::Color::White);
    Text.setStyle(sf::Text::Bold);
}



void DefinirText(std::string text, sf::Text &Label, sf::Font &Font, int x, int y){

    SetText(Label, Font, x, y);
    Label.setString(text);
    Label.setCharacterSize(15);

    return;
}


int main() {
    sf::RenderWindow window(sf::VideoMode(900, 540), "Tetris game");
    window.setActive(true);

    window.setFramerateLimit(60);  
    sf::Texture TextTruc, TextWall,StatText, FondPrincipal;

    sf::RenderTexture renderTexture;
    if (!renderTexture.create(900, 540)) {
        std::cerr << "Impossible de créer la RenderTexture" << std::endl;
        return -1;
    }
    sf::Font font;

    if (!font.loadFromFile("asset/arial.ttf")) {
        EXIT_FAILURE;
    }

    sf::Text textScore,textNiveau,textNextPiece,textLignes,textCombo;
    SetText(textCombo, font, 0, 0);
    textCombo.setFont(font);
    textCombo.setCharacterSize(25);
    textCombo.setStyle(sf::Text::Bold);


    if (!FondPrincipal.loadFromFile("asset/FondPrincipal.png")) { 
        std::cerr << "Erreur : Im345possible de charger l'image.\n";
        return -1;
    }

    
    sf::Sprite SpStat(StatText), FondP(FondPrincipal);

    SpStat.setScale(0.65f, 0.65f);
    SpStat.setPosition(100, 200);

    FondP.setPosition(0,0);

    DefinirText("Prochaine piece : ", textNextPiece, font, 582, 70);
    
    SetText(textScore, font, 180, 335);
    SetText(textNiveau, font, 110, 240);
    SetText(textLignes, font, 180, 400);


    if (!TextTruc.loadFromFile("asset/tiles.png")){
        return EXIT_FAILURE;
    }
   
    int ValeurY=0;

    menu Menu(window, font);
    window.clear();

    int MenuOptions = Menu.MenuJeu();
    while(window.isOpen()){
        MonblocCopy = nullptr;
        sf::Event event;
        bloc Monbloc(TextTruc, &window, 360, 136 );        
        MonblocCopy = &Monbloc;
        Monbloc.BlocAleatoire(); Monbloc.CouleurAleatoire(); Monbloc.RegenererBloc();
        bool ThreadLance = false;
        while(MenuOptions == 1){
            sf::Clock gravityClock;
            while(!MonblocCopy->Perdu()){            

                if (gravityClock.getElapsedTime().asMilliseconds() > Monbloc.VitesseBloc()) {
                    
                if(!Monbloc.checkmove(0, 1)){
                    int lignes = Monbloc.ClearLines();
                    if(lignes > 0){
                        Monbloc.ScoreAdd("Ligne", lignes);
                    }
                    Monbloc.ResetBloc();
                    gravityClock.restart();
                } else {
                    Monbloc.mouvement("down");
                    gravityClock.restart();
                }
                                    
                    gravityClock.restart();
                }

                while(window.pollEvent(event)) {
                    if (event.type == sf::Event::Closed) {
                        window.close();
                        break;
                    }

                    if (event.type == sf::Event::KeyPressed) {

                        if(event.key.code == sf::Keyboard::Enter){
                            Monbloc.RotationBloc();
                        }

                        if(event.key.code == sf::Keyboard::Right){
                            Monbloc.mouvement("right");
                        }

                        if(event.key.code == sf::Keyboard::Left){
                            Monbloc.mouvement("left");
                        }

                        if(event.key.code == sf::Keyboard::Down){
                            if(!Monbloc.DetectionBlocEmpile()){
                                Monbloc.mouvement("down");
                                Monbloc.ScoreAdd("DescenteRapide", 0);
                            }
                        }

                        if(event.key.code == sf::Keyboard::Space){
                            Monbloc.AtterirEnBas();
                        }

                        if(event.key.code == sf::Keyboard::RShift){
                            Monbloc.ChangerBloc();
                        }
                    }
                }





                renderTexture.clear();
                Monbloc.ChangementNiveau();
                textScore.setString(Monbloc.AfficherScore());
                textNiveau.setString(Monbloc.AfficherNiveau());
                textLignes.setString(Monbloc.AfficherLigneDetruite());
                window.clear(sf::Color(15, 15, 15));
                window.draw(FondP);

                sf::RectangleShape limiteLine(sf::Vector2f(180.f, 2.f));
                limiteLine.setFillColor(sf::Color(255, 0, 0, 150));
                limiteLine.setPosition(360.f, 208.f);
                window.draw(limiteLine);

                Monbloc.DessinerLeTableau();
                Monbloc.next(); Monbloc.Saved();
                window.draw(textNiveau); window.draw(textLignes); window.draw(textScore);

                textCombo.setOutlineThickness(2.0f);
                textCombo.setOutlineColor(sf::Color::Black);

                Monbloc.UpdateCombo();
                int comboVal = Monbloc.AfficherCombo();

                if(comboVal > 0) {
                    textCombo.setString("COMBO X" + std::to_string(comboVal));
                    
                    float time = gravityClock.getElapsedTime().asSeconds();
                    float scale = 1.0f + std::sin(time * 10.0f) * 0.1f; 
                    textCombo.setScale(scale, scale);
                    textCombo.setRotation(-5.0f);

                    sf::Color color;
                    if(comboVal >= 8)      color = sf::Color(255, 0, 255); // Violet (Elite)
                    else if(comboVal >= 5) color = sf::Color(255, 50, 50);  // Rouge (Chaud)
                    else if(comboVal >= 3) color = sf::Color(255, 165, 0); // Orange
                    else                   color = sf::Color::Cyan;        // Départ

                    textCombo.setFillColor(color);

                    sf::FloatRect cbounds = textCombo.getLocalBounds();
                    textCombo.setOrigin(cbounds.width / 2.0f, cbounds.height / 2.0f);
                    textCombo.setPosition(450.f, 155.f); 

                    window.draw(textCombo);


                    float tempsRestant = Monbloc.TempsRestantCombo();
                    float ratio = std::max(0.f, std::min(1.f, tempsRestant / Monbloc.GetComboTimeLimit()));
                    
                    float barWidth = 140.f;
                    float barHeight = 8.f;
                    float rotationAngle = -5.0f;
                    sf::Vector2f barPos(450.f, 185.f); // Position centrale sous le texte

                    // 1. LE FOND (Contour + Arrière-plan sombre)
                    sf::RectangleShape barFond(sf::Vector2f(barWidth, barHeight));
                    barFond.setOrigin(barWidth / 2.f, barHeight / 2.f);
                    barFond.setPosition(barPos);
                    barFond.setRotation(rotationAngle);
                    barFond.setFillColor(sf::Color(0, 0, 0, 150));
                    barFond.setOutlineThickness(1.5f);
                    barFond.setOutlineColor(sf::Color(255, 255, 255, 80)); // Petit liseré gris/blanc
                    window.draw(barFond);

                    // 2. LE REMPLISSAGE (La partie qui diminue)
                    if (ratio > 0.01f) {
                        sf::RectangleShape barFill(sf::Vector2f(barWidth * ratio, barHeight));
                        
                        // ASTUCE : L'origine à gauche permet de faire diminuer la barre vers la gauche facilement
                        barFill.setOrigin(0.f, barHeight / 2.f); 
                        
                        // On calcule la position de départ (le bord gauche de la barre inclinée)
                        // Pour simplifier, on garde l'origine au centre du fond et on décale le Fill
                        barFill.setRotation(rotationAngle);
                        
                        // Calcul du décalage pour que le bord gauche du "Fill" s'aligne avec le bord gauche du "Fond"
                        // On recule de la moitié de la largeur totale
                        float rad = rotationAngle * 3.14159f / 180.f;
                        float offsetX = (barWidth / 2.f) * std::cos(rad);
                        float offsetY = (barWidth / 2.f) * std::sin(rad);
                        
                        barFill.setPosition(barPos.x - offsetX, barPos.y - offsetY);

                        // Couleur dynamique (Dégradé visuel)
                        sf::Color fillCol;
                        if(ratio > 0.5f)      fillCol = sf::Color(0, 255, 150); // Vert Emeraude
                        else if(ratio > 0.2f) fillCol = sf::Color(255, 200, 0); // Jaune/Orange
                        else                  fillCol = sf::Color(255, 50, 50);  // Rouge flash

                        barFill.setFillColor(fillCol);
                        window.draw(barFill);

                        // 3. PETIT EFFET DE BRILLANCE (Glossy effect)
                        sf::RectangleShape barShine(sf::Vector2f(barWidth * ratio, barHeight / 2.f));
                        barShine.setOrigin(0.f, barHeight / 4.f);
                        barShine.setPosition(barPos.x - offsetX, barPos.y - offsetY);
                        barShine.setRotation(rotationAngle);
                        barShine.setFillColor(sf::Color(255, 255, 255, 50));
                        window.draw(barShine);
                    }
                }
                
                Monbloc.VisualiserBloc();
                window.display();
                
            
                float centerXscore = (650 + 827) / 2.0f, centerYScore = 282;
                sf::FloatRect textBoundsScore = textScore.getLocalBounds();
                textScore.setOrigin(textBoundsScore.left + textBoundsScore.width / 2.0f, textBoundsScore.top + textBoundsScore.height / 2.0f);
                textScore.setPosition(centerXscore, centerYScore);

                float centerXlvl = (66 + 246) / 2.0f, centerYlvl = 470;
                sf::FloatRect textBoundsLvl = textNiveau.getLocalBounds();
                textNiveau.setOrigin(textBoundsLvl.left + textBoundsLvl.width / 2.0f, textBoundsLvl.top + textBoundsLvl.height / 2.0f);
                textNiveau.setPosition(centerXlvl, centerYlvl);

                float centerXLigne = (66 + 246) / 2.0f, centerYLigne = 345;
                sf::FloatRect textBoundsLignes = textLignes.getLocalBounds();
                textLignes.setOrigin(textBoundsLignes.left + textBoundsLignes.width / 2.0f, textBoundsLignes.top + textBoundsLignes.height / 2.0f);
                textLignes.setPosition(centerXLigne, centerYLigne);

            }

            MonblocCopy = nullptr;
            
            sf::Texture textureFond;
            textureFond.create(900,540);
            textureFond.update(window);       
            int ChoixMenuPerdu = Menu.MenuPerdu(Monbloc.AfficherScore(), textureFond);

            if(ChoixMenuPerdu == 1){
                if(window.isOpen()) window.close();
            }else{
                break;
            }
            
        }
        if(MenuOptions == 0){
            window.close();
            MonblocCopy = nullptr;
        }
    }    
    return 0;
} 