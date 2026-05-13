#include "menu.h"
#include <iostream>


const char* blur_frag = R"(
uniform sampler2D texture;
uniform vec2 resolution;
uniform float radius;

const float PI = 3.1415926535897932384626433832795;

float gaussian(float x, float sigma) {
    return exp(-(x * x) / (2.0 * sigma * sigma)) / (2.0 * PI * sigma * sigma);
}

void main() {
    vec2 texCoord = gl_TexCoord[0].xy;
    vec4 color = vec4(0.0);
    
    float sigma = radius / 2.0;
    float sum = 0.0;
    
    for (float x = -radius; x <= radius; x++) {
        for (float y = -radius; y <= radius; y++) {
            vec2 offset = vec2(x, y) / resolution;
            float weight = gaussian(length(vec2(x, y)), sigma);
            color += texture2D(texture, texCoord + offset) * weight;
            sum += weight;
        }
    }
    
    gl_FragColor = color / sum;
})";


menu::menu(sf::RenderWindow& window, sf::Font &font1){
    AddrWindow = &window;
    font = &font1;
    if (!blurShader.loadFromMemory(blur_frag, sf::Shader::Fragment)) throw std::runtime_error("Failed to load shader");
    blurShader.setUniform("resolution", sf::Vector2f(900, 540));
    blurShader.setUniform("radius", 5.f);   
}

menu::~menu(){}


void centrerTexte(sf::Text& texte) {
    sf::FloatRect bounds = texte.getLocalBounds();
    texte.setOrigin(bounds.left + bounds.width / 2.0f, 
                    bounds.top + bounds.height / 2.0f);
}

void menu::Flou(sf::Texture& texture){
    sf::Sprite sprite(texture);
    AddrWindow->clear();
    AddrWindow->draw(sprite, &blurShader);
}

void menu::Pause(sf::Texture& texture){
    sf::Text Pause;
    Pause.setFont(*font);
    Pause.setCharacterSize(20);
    Pause.setPosition(370.0,230.0);
    Pause.setFillColor(sf::Color::White);
    Pause.setStyle(sf::Text::Bold);
    Pause.setString("         Pause \n O pour continuer");
    Flou(texture);
    AddrWindow->draw(Pause); 
}


int menu::MenuJeu(){

    sf::Event event;
    sf::Text Pause, Quitter;
    sf::Texture TextLogo, TextFond;

    if (!TextLogo.loadFromFile("asset/TetrisLogo.png")) return EXIT_FAILURE;
    if (!TextFond.loadFromFile("asset/Fond.png")) return EXIT_FAILURE;
    

    sf::Sprite Logo(TextLogo);
    sf::Sprite Fond(TextFond), Fond2(TextFond);

    Pause.setFont(*font);
    Pause.setString("Jouer !");
    Pause.setCharacterSize(30);
    Pause.setFillColor(sf::Color::White);
    sf::FloatRect PausetextBounds = Pause.getLocalBounds(); 
    sf::Vector2u windowSize = AddrWindow->getSize(); 

    Pause.setOrigin(PausetextBounds.width / 2, PausetextBounds.height / 2); 
    Pause.setPosition(windowSize.x / 2, windowSize.y / 2); 

    Quitter.setFont(*font);
    Quitter.setString("Quitter");
    Quitter.setCharacterSize(30);
    Quitter.setFillColor(sf::Color::White);
    sf::FloatRect QuittertextBounds = Quitter.getLocalBounds();
    Quitter.setOrigin(QuittertextBounds.width / 2, QuittertextBounds.height / 2); 
    Quitter.setPosition(windowSize.x / 2, (windowSize.y / 2)+100); 


    sf::FloatRect LogoBound = Logo.getLocalBounds();
    Logo.setOrigin(LogoBound.width/2, LogoBound.height/2);
    Logo.setPosition(windowSize.x/2, 100);

    sf::Clock gameClock;

    Fond.setPosition(0, 0);
    Fond2.setPosition(0, TextFond.getSize().y);

    float scrollSpeed = 25.0f;

    while (true && AddrWindow->isOpen()) {
        while (AddrWindow->pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                AddrWindow->close();
                return 0; 
            }

            sf::FloatRect PausetextGBounds = Pause.getGlobalBounds(); 
            sf::FloatRect QuittertextGBounds = Quitter.getGlobalBounds();
            sf::Vector2i mousePos = sf::Mouse::getPosition(*AddrWindow);
            sf::Vector2f mousePosF(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
            std::vector<sf::Text*> boutons = { &Pause, &Quitter };

            for (auto* btn : boutons) {
                if (btn->getGlobalBounds().contains(mousePosF)) {
                    btn->setFillColor(sf::Color(255, 204, 0));
                    btn->setScale(1.1f, 1.1f);
                } else {
                    btn->setFillColor(sf::Color::White);
                    btn->setScale(1.0f, 1.0f);
                }
            }
            
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                if (PausetextGBounds.contains(mousePosF)) return 1; 
                if (QuittertextGBounds.contains(mousePosF)) return 0;
            }
        }
        float deltaTime = gameClock.restart().asSeconds();
        float move = scrollSpeed * deltaTime;

        Fond.move(0, move);
        Fond2.move(0, move);

        if (Fond.getPosition().y >= AddrWindow->getSize().y) {
            Fond.setPosition(0, Fond2.getPosition().y - TextFond.getSize().y);
        }
        if (Fond2.getPosition().y >= AddrWindow->getSize().y) {
            Fond2.setPosition(0, Fond.getPosition().y - TextFond.getSize().y);
        }

        AddrWindow->clear();
        AddrWindow->draw(Fond);
        AddrWindow->draw(Fond2);
        AddrWindow->draw(Pause);
        AddrWindow->draw(Quitter);
        AddrWindow->draw(Logo);
        AddrWindow->display();
    }
    return 0; 
}



int menu::MenuPerdu(std::string score, sf::Texture& texture){
    sf::Event event;
    sf::Text PerduTxt("Vous avez perdu !", *font, 50),
             Quitter("Quitter", *font, 35),
             Recommencer("Recommencer", *font, 35),
             Score("Votre score : "+score, *font, 35);

    sf::FloatRect boundsPerdu = PerduTxt.getLocalBounds();
    sf::FloatRect boundsQuit = Quitter.getLocalBounds();
    sf::FloatRect boundsRestart = Recommencer.getLocalBounds();
    sf::FloatRect boundsScore = Score.getLocalBounds();

    float x = (AddrWindow->getSize().x - boundsPerdu.width) / 2 - boundsPerdu.left;
    float x1 = (AddrWindow->getSize().x - boundsQuit.width) / 2 - boundsQuit.left;
    float x2 = (AddrWindow->getSize().x - boundsRestart.width) / 2 - boundsRestart.left;
    float x3 = (AddrWindow->getSize().x - boundsScore.width) / 2 - boundsScore.left;

    PerduTxt.setPosition(x, 150);
    Quitter.setPosition(x1, 330);
    Recommencer.setPosition(x2, 240);
    Score.setPosition(x3, 400);

    AddrWindow->setFramerateLimit(60);
    while(AddrWindow->isOpen()){
        while (AddrWindow->pollEvent(event)) {
            if (event.type == sf::Event::Closed){
                AddrWindow->close();
                return 0; 
            }
            sf::FloatRect RestarttextGBounds = Recommencer.getGlobalBounds(); 
            sf::FloatRect QuittertextGBounds = Quitter.getGlobalBounds();
            sf::Vector2i mousePos = sf::Mouse::getPosition(*AddrWindow);
            sf::Vector2f mousePosF(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
            std::vector<sf::Text*> boutons = { &Recommencer, &Quitter };

            for (auto* btn : boutons) {
                if (btn->getGlobalBounds().contains(mousePosF)) {
                    btn->setFillColor(sf::Color(255, 204, 0));
                    btn->setScale(1.1f, 1.1f);
                } else {
                    btn->setFillColor(sf::Color::White);
                    btn->setScale(1.0f, 1.0f);
                }
            }
            
            
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                if (RestarttextGBounds.contains(mousePosF)) return 0; 
                if (QuittertextGBounds.contains(mousePosF)) return 1;
            }
        }

        AddrWindow->clear();
        Flou(texture);
        AddrWindow->draw(PerduTxt);
        AddrWindow->draw(Quitter);
        AddrWindow->draw(Recommencer);
        AddrWindow->draw(Score);
        AddrWindow->display();
    }
    
    return 0;
}