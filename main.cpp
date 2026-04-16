#include "bloc.h"
#include "menu.h"
#ifndef SFML_STATIC
#define SFML_STATIC
#endif
#include <SFML/Graphics.hpp>


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

    sf::Text textScore,textNiveau,textNextPiece,textLignes;


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

                while (gravityClock.getElapsedTime().asMilliseconds() > Monbloc.VitesseBloc()) {

                    Monbloc.mouvement("down");

                    if(Monbloc.DetectionBlocEnBas() || Monbloc.DetectionBlocEmpile()){   

                        int LigneTmp = 0;
                        int lignes = Monbloc.ClearLines();

                        if(lignes > 0){
                            Monbloc.ScoreAdd("Ligne", lignes);
                        }

                        Monbloc.ResetBloc();
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

                Monbloc.DessinerLeTableau();
                Monbloc.next(); Monbloc.Saved();
                window.draw(textNiveau); window.draw(textLignes); window.draw(textScore);
                
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
            Monbloc.~bloc();
            MonblocCopy = nullptr;
        }
    }    
    return 0;
} 