#include "bloc.h"
#ifndef SFML_STATIC
#define SFML_STATIC
#endif

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <cstdlib>
#include <ctime>

#include <iostream>


bloc::bloc(const sf::Texture& TextTruc, sf::RenderWindow* window, int initialY, int initialX): AddrWindow(window), Tiles(TextTruc){
    std::srand(std::time(0));
    InitialiserPOS();
    CouleurAleatoire();
    BlocAleatoire();
    Tiles.setTextureRect(sf::IntRect(0,0,18,18));
    Tiles.setPosition(initialX, initialY);
    TabY = initialY;
    TabX = initialX;
    VPerdu = false;

    int PatterneA[28] = { 
                 0,2,4,6,   //I
                 0,2,4,5,    // L
                 1,3,4,5,   // L reversed      
                 0,2,3,4,   // T 
                 0,2,3,5,    // Z
                 1,2,3,4,     // Z reversed             
                 0,1,2,3     // carré
    };

    for(int i = 0; i < 28; i++) PatterneApp[i] = PatterneA[i];
    for(int i = 0; i <28;  i++) Patterne[i] = PatterneApp[i];

    //Mise en place de la map colonne ligne  (à inverser LOL)
    for(int i = 0; i <20;  i++) for(int j = 0; j<10; j++) map[i][j]= 0;
}

bloc::~bloc(){

}

void bloc::DefinitionDeStruct(int Plage, int Pattern, int i, int j){
    switch (Plage){
    case 0:
        PosTot.X1 = i;
        PosTot.Y1 = j;
        break;
    case 1:
        PosTot.X2 = i;
        PosTot.Y2 = j;
        break;
    case 2:

        PosTot.X3 = i;
        PosTot.Y3 = j;
        break;
    case 3:

        PosTot.X4 = i;
        PosTot.Y4 = j;
        break;
    default:
        return;
        break;
    }
}



void bloc::assembly(){
    const int KpatternInit = NbBloc*4;
    int Kpattern = KpatternInit, tab1D[8];
    for(int i=0;i<8;i++){
        if(Patterne[Kpattern]==i){
            tab1D[i]= CouleurAlea; 
            Kpattern++;
        }else tab1D[i]=0; 
    }
    int k=0, w=0;
    for(int i=0;i<4;i++)for(int j=3;j<5;j++){
        if(tab1D[k] >= 1){
            DefinitionDeStruct(w,KpatternInit,j,i);
            w++;
        }
        map[i][j]=tab1D[k++];
    }
}

void bloc::next(){   
    const int KpatternInit = NbBlocSuivant*4;
    int Kpattern = KpatternInit, k=0;
    int tab1D[8],tab2D[4][2];
    for(int i=0;i<8;i++){
        if(Patterne[Kpattern]==i){
            tab1D[i]= CouleurAleaSuivant; 
            Kpattern++;
        }else tab1D[i]=0; 
    }     

    for(int i=0;i<4;i++)for(int j=0;j<2;j++) tab2D[i][j]=tab1D[k++];

    int X, Y;
    if(NbBlocSuivant == 0){
        X = ((813-656)/2)+655-9;
        Y = ((190-75)/2)+75 - 36;
    }else{
        X = ((813-656)/2)+655-18;
        if(NbBlocSuivant == 6){
            Y = ((190-75)/2)+75 - 16;
        }else{
            Y = ((190-75)/2)+75 - 27;
        }
    }

    
    for(int i=0;i<4;i++)for(int j=0;j<2;j++){        
        if(tab2D[i][j] >= 1){
            Tiles.setTextureRect(sf::IntRect(18*CouleurAleaSuivant,0,18,18));
            Tiles.setPosition(sf::Vector2f((X+(j*18)),(Y+(i*18))));
            AddrWindow->draw(Tiles);
        }
    }

}


bool bloc::checkLine(){
    for(int y = 0; y < 20; y++){
        int compteur = 0;

        for(int x = 0; x < 10; x++){
            if(map[y][x] > 0){
                compteur++;
            } else {
                break;
            }
        }

        if(compteur == 10){
            LigneComplete = y;
            return true;
        }
    }
    return false;
}

bool bloc::checkmove(int dx, int dy){
    const int* X[] = {&PosTot.X1, &PosTot.X2, &PosTot.X3, &PosTot.X4};
    const int* Y[] = {&PosTot.Y1, &PosTot.Y2, &PosTot.Y3, &PosTot.Y4};

    for(int i = 0; i < 4; i++){
        int newX = *X[i] + dx;
        int newY = *Y[i] + dy;

        if(newX < 0 || newX >= 10 || newY < 0 || newY >= 20)
            return false;

        if(map[newY][newX] != 0){
            bool isSelf = false;
            for(int j = 0; j < 4; j++){
                if(newX == *X[j] && newY == *Y[j]){
                    isSelf = true;
                    break;
                }
            }
            if(!isSelf) return false;
        }
    }
    return true;
}

void bloc::mouvement(std::string NomMouv){   
    if(NomMouv == "left") DeplacementGauche();
    if(NomMouv == "right") DeplacementDroite();
    if(NomMouv == "down") DeplacementBas();
    
}

void bloc::DrawTiles(){
    AddrWindow->draw(Tiles);
}

void bloc::drawASprite(sf::Sprite &Tile){
    AddrWindow->draw(Tile);
}


void bloc::DessinerLeTableau(){
    for(int i = 0; i<20; i++){ 
        for(int j = 0; j<10; j++){
            if (map[i][j] >= 1) {
                int Couleur = map[i][j];
                Tiles.setTextureRect(sf::IntRect(18*Couleur,0,18,18));
                Tiles.setPosition(sf::Vector2f(TabY+18*j,TabX+18*i));
                DrawTiles();
            }
        }
    }
    
}

void bloc::Ajouter(int X, int Y, int  Nbr){
    map[Y][X] = Nbr;
}

void bloc::DeplacementBas(){
    if(!checkmove(0, 1)) return;

    Ajouter(PosTot.X1,PosTot.Y1, 0);
    Ajouter(PosTot.X2,PosTot.Y2, 0);
    Ajouter(PosTot.X3,PosTot.Y3, 0);
    Ajouter(PosTot.X4,PosTot.Y4, 0);

    PosTot.Y1++; PosTot.Y2++; PosTot.Y3++; PosTot.Y4++;

    Ajouter(PosTot.X1,PosTot.Y1, CouleurAlea);
    Ajouter(PosTot.X2,PosTot.Y2, CouleurAlea);
    Ajouter(PosTot.X3,PosTot.Y3, CouleurAlea);
    Ajouter(PosTot.X4,PosTot.Y4, CouleurAlea);
}

void bloc::DeplacementDroite(){
    if(!checkmove(1, 0)) return;

    Ajouter(PosTot.X1,PosTot.Y1, 0);
    Ajouter(PosTot.X2,PosTot.Y2, 0);
    Ajouter(PosTot.X3,PosTot.Y3, 0);
    Ajouter(PosTot.X4,PosTot.Y4, 0);

    PosTot.X1++; PosTot.X2++; PosTot.X3++; PosTot.X4++;

    Ajouter(PosTot.X1,PosTot.Y1, CouleurAlea);
    Ajouter(PosTot.X2,PosTot.Y2, CouleurAlea);
    Ajouter(PosTot.X3,PosTot.Y3, CouleurAlea);
    Ajouter(PosTot.X4,PosTot.Y4, CouleurAlea);
}

void bloc::DeplacementGauche(){
    if(!checkmove(-1, 0)) return;

    Ajouter(PosTot.X1,PosTot.Y1, 0);
    Ajouter(PosTot.X2,PosTot.Y2, 0);
    Ajouter(PosTot.X3,PosTot.Y3, 0);
    Ajouter(PosTot.X4,PosTot.Y4, 0);

    PosTot.X1--; PosTot.X2--; PosTot.X3--; PosTot.X4--;

    Ajouter(PosTot.X1,PosTot.Y1, CouleurAlea);
    Ajouter(PosTot.X2,PosTot.Y2, CouleurAlea);
    Ajouter(PosTot.X3,PosTot.Y3, CouleurAlea);
    Ajouter(PosTot.X4,PosTot.Y4, CouleurAlea);
}

void bloc::InitialiserPOS(){    
    if (PosTot.Y1 == 0) VPerdu = true;
    PosTot.X1=0; PosTot.X2=0; PosTot.X3=0; PosTot.X4=0;
    PosTot.Y1=0; PosTot.Y2=0; PosTot.Y3=0; PosTot.Y4=0;
}

void bloc::RegenererBloc(){
    NbBloc = NbBlocSuivant;
    CouleurAlea = CouleurAleaSuivant;
    CouleurAleatoire();
    BlocAleatoire();
    assembly();
}

void bloc::CouleurAleatoire(){
    CouleurAleaSuivant = rand() %6 + 1;
}

void bloc::BlocAleatoire(){
    NbBlocSuivant = rand() % 7;
}

bool bloc::DetectionBlocEnBas(){
    if(PosTot.Y4 == 19 || PosTot.Y1 == 19 || PosTot.Y2 == 19 || PosTot.Y3 == 19) return true;
    
    return false;
}

bool bloc::DetectionBlocEmpile(){
    if(map[PosTot.Y1+1][PosTot.X1] >= 1 && !( ((PosTot.Y2 == PosTot.Y1+1) && (PosTot.X2 == PosTot.X1)) || ((PosTot.Y1+1 == PosTot.Y3) && (PosTot.X1 == PosTot.X3)) || ((PosTot.Y1+1 == PosTot.Y4) && (PosTot.X1 == PosTot.X4)) ) ) return true;
    if(map[PosTot.Y2+1][PosTot.X2] >= 1 && !( ((PosTot.Y2+1 == PosTot.Y1) && (PosTot.X2 == PosTot.X1)) || ((PosTot.Y2+1 == PosTot.Y3) && (PosTot.X2 == PosTot.X3)) || ((PosTot.Y2+1 == PosTot.Y4) && (PosTot.X2 == PosTot.X4)) ) ) return true;
    if(map[PosTot.Y3+1][PosTot.X3] >= 1 && !( ((PosTot.Y3+1 == PosTot.Y1) && (PosTot.X3 == PosTot.X1)) || ((PosTot.Y3+1 == PosTot.Y2) && (PosTot.X3 == PosTot.X2)) || ((PosTot.Y3+1 == PosTot.Y4) && (PosTot.X3 == PosTot.X4)) ) ) return true;
    if(map[PosTot.Y4+1][PosTot.X4] >= 1 && !( ((PosTot.Y4+1 == PosTot.Y1) && (PosTot.X4 == PosTot.X1)) || ((PosTot.Y4+1 == PosTot.Y3) && (PosTot.X4 == PosTot.X3)) || ((PosTot.Y4+1 == PosTot.Y2) && (PosTot.X4 == PosTot.X2)) ) ) return true;
    return false;
}

void bloc::ResetBloc(){
    DejaSave = false;
    int LigneTmp =0;  
    
    const int* Y[] = {&PosTot.Y1, &PosTot.Y2, &PosTot.Y3, &PosTot.Y4};
    
    for(int i = 0; i < 4 ; i++){
        if(*Y[i] <= 3 ){
            VPerdu = true;
            return;
        }
    }

    rotation =0;
    InitialiserPOS();
    RegenererBloc();
    
}

void bloc::VoirLeTableau(){
    std::cout <<"\n\n";
    for(int i =0; i <20 ; i++){
        for(int j=0 ; j<10; j++){
            std::cout << map[i][j];
        }       
        std::cout << "\n";
    }
}

void bloc::ViderTableau(){
    for(int i=0; i<20; i++){
        for(int j=0; j<10; j++) map[i][j] = 0;
    }
}

bool bloc::Perdu(){
    return VPerdu;
}

int bloc::ClearLines(){
    int lines = 0;

    for(int y = 19; y >= 0; y--){
        bool full = true;

        for(int x = 0; x < 10; x++){
            if(map[y][x] == 0){
                full = false;
                break;
            }
        }

        if(full){
            lines++;

            // descendre tout
            for(int yy = y; yy > 0; yy--){
                for(int x = 0; x < 10; x++){
                    map[yy][x] = map[yy-1][x];
                }
            }

            // vider top
            for(int x = 0; x < 10; x++){
                map[0][x] = 0;
            }

            y++; // recheck même ligne
        }
    }

    return lines;
}

void bloc::ScoreAdd(std::string TypePts, int Nbr){
    if(TypePts == "Ligne"){
        int PtsBase = Nbr *100;
        
        int ScoreTmp = PtsBase + (100*(Nbr-1));
        score += ScoreTmp;
    }

    else if(TypePts == "DescenteRapide"){
        
        score += (Niveau+1)*1;
    }

    else if(TypePts == "DescenteNow"){
        score += (Niveau+1)*2;
    }
}

void bloc::ChangementNiveau(){
    if(Niveau == 29) return;
    if((Niveau+1)*4 <= LigneDetruite){
        Niveau++;
        LigneDetruite = 0;
    }
}

void bloc::RotationBloc(){
    if(NbBloc == 6) return; // carré ne tourne pas duh

    int pivotX = PosTot.X2;
    int pivotY = PosTot.Y2;

    int newX[4];
    int newY[4];

    const int X[] = {PosTot.X1, PosTot.X2, PosTot.X3, PosTot.X4};
    const int Y[] = {PosTot.Y1, PosTot.Y2, PosTot.Y3, PosTot.Y4};

    // Calcul rotation
    for(int i = 0; i < 4; i++){
        newX[i] = pivotX - (Y[i] - pivotY);
        newY[i] = pivotY + (X[i] - pivotX);
    }

    // Vérification collision
    for(int i = 0; i < 4; i++){
        if(newX[i] < 0 || newX[i] >= 10 || newY[i] < 0 || newY[i] >= 20)
            return;

        if(map[newY[i]][newX[i]] != 0){
            bool isSelf = false;
            for(int j = 0; j < 4; j++){
                if(newX[i] == X[j] && newY[i] == Y[j]){
                    isSelf = true;
                    break;
                }
            }
            if(!isSelf) return;
        }
    }

    // Efface ancien bloc
    Ajouter(PosTot.X1,PosTot.Y1,0);
    Ajouter(PosTot.X2,PosTot.Y2,0);
    Ajouter(PosTot.X3,PosTot.Y3,0);
    Ajouter(PosTot.X4,PosTot.Y4,0);

    // Applique rotation
    PosTot.X1 = newX[0]; PosTot.Y1 = newY[0];
    PosTot.X2 = newX[1]; PosTot.Y2 = newY[1];
    PosTot.X3 = newX[2]; PosTot.Y3 = newY[2];
    PosTot.X4 = newX[3]; PosTot.Y4 = newY[3];

    // Réécrit dans map
    Ajouter(PosTot.X1,PosTot.Y1,CouleurAlea);
    Ajouter(PosTot.X2,PosTot.Y2,CouleurAlea);
    Ajouter(PosTot.X3,PosTot.Y3,CouleurAlea);
    Ajouter(PosTot.X4,PosTot.Y4,CouleurAlea);


    // rien de bien sorcier, un wallkick de base
    for(int shift = -1; shift <= 1; shift++){
        bool ok = true;

        for(int i = 0; i < 4; i++){
            int testX = newX[i] + shift;

            if(testX < 0 || testX >= 10){
                ok = false;
                break;
            }
        }

        if(ok){
            for(int i = 0; i < 4; i++){
                newX[i] += shift;
            }
            break;
        }
    }
}


void bloc::ChangerBloc(){

    if(DejaSave) return;

    DejaSave = true;

    rotation = 0;
    if(PosTot.Y1 >= 5) return;

    switch (BlocSaved){
        case 8:
            BlocSaved = NbBloc;
            NbBloc = NbBlocSuivant;

            CouleurSaved = CouleurAlea;
            CouleurAlea = CouleurAleaSuivant;

            RemplacerBlocSave(); 
            break;
        
        default:
            int BlocTmp = NbBloc;
            NbBloc = BlocSaved;
            BlocSaved = BlocTmp;

            int ColorTmp = CouleurAlea;
            CouleurAlea = CouleurSaved;
            CouleurSaved = ColorTmp;

            RemplacerBlocSave();
            break;
    }
}


void bloc::RemplacerBlocSave(){
    
    map[PosTot.X1][PosTot.Y1] = 0;
    map[PosTot.X2][PosTot.Y2] = 0;
    map[PosTot.X3][PosTot.Y3] = 0;
    map[PosTot.X4][PosTot.Y4] = 0;
    assembly();
}

void bloc::Saved(){
    if(BlocSaved == 8) return;
    const int KpatternInit = BlocSaved*4;
    int Kpattern = KpatternInit, k=0;
    int tab1D[8],tab2D[4][2];
    for(int i=0;i<8;i++){
        if(Patterne[Kpattern]==i){
            tab1D[i]= CouleurSaved; 
            Kpattern++;
        }else tab1D[i]=0; 
    }     
    for(int i=0;i<4;i++)for(int j=0;j<2;j++) tab2D[i][j]=tab1D[k++];

    int X, Y;
    if(BlocSaved == 0){
        X = ((237-80)/2)+80;
        Y = ((252-138)/2)+138 - 36;
    }else{
        X = ((237-80)/2)+80-18;
        if(BlocSaved == 6){
            Y = ((252-138)/2)+138 - 16;
        }else{
            Y = ((252-138)/2)+138 - 27;
        }
    }


    for(int i=0;i<4;i++)for(int j=0;j<2;j++){        
        if(tab2D[i][j] >= 1){
            Tiles.setTextureRect(sf::IntRect(18*CouleurSaved,0,18,18));
            Tiles.setPosition(sf::Vector2f((X+(j*18)),(Y+(i*18))));
            AddrWindow->draw(Tiles);
        }
    }
}

void bloc::AtterirEnBas(){
    
    for (int i = 0; i < 20; i++){
        if(!DetectionBlocEmpile()){
            DeplacementBas();    
            if (Niveau == 0) score += 2;
            else score += Niveau*2;
        }
    }
    ResetBloc();
    
}

void bloc::VisualiserBloc(){

    sf::Texture texture;
    if (!texture.loadFromFile("asset/tiles.png")) {
        EXIT_FAILURE;
    }
    sf::Sprite sprite;
    sprite.setTexture(texture);
    sprite.setTextureRect(sf::IntRect(18*CouleurAlea,0,18,18));
    sprite.setColor(sf::Color(255, 255, 255, 128)); // Opacité à 50%



    const int* X[] = {&PosTot.X1, &PosTot.X2, &PosTot.X3, &PosTot.X4};
    const int* Y[] = {&PosTot.Y1, &PosTot.Y2, &PosTot.Y3, &PosTot.Y4};
    int XPlusGrand= *X[0], XPlusPetit= *X[0], YPlusGrand= *Y[0];

    for (int i = 0; i < 4; i++){
        if(*X[i]>XPlusGrand) XPlusGrand = *X[i];
    }
    for (int i = 0; i < 4; i++){
        if(*X[i]<XPlusPetit) XPlusPetit = *X[i];
    }
    
    for (int i = 0; i < 4; i++){
        if(*Y[i]>YPlusGrand){
            YPlusGrand = *Y[i];
        }
    }

    for(int i=YPlusGrand; i<20; i++){


        int Y = i-YPlusGrand;
        if(
            (map[PosTot.Y1 + Y][PosTot.X1] == 0) &&
            (map[PosTot.Y2 + Y][PosTot.X2] == 0) &&
            (map[PosTot.Y3 + Y][PosTot.X3] == 0) &&
            (map[PosTot.Y4 + Y][PosTot.X4] == 0)
        && 
            ((
                (map[PosTot.Y1 + Y+1][PosTot.X1] >=1)||
                (map[PosTot.Y2 + Y+1][PosTot.X2] >=1)||
                (map[PosTot.Y3 + Y+1][PosTot.X3] >=1)||
                (map[PosTot.Y4 + Y+1][PosTot.X4] >=1)
            
            )
            || 
                (i==19)
            )

        ){
            sprite.setPosition(360+18*PosTot.X1,136+18*(PosTot.Y1+Y));
            AddrWindow->draw(sprite);
            sprite.setPosition(360+18*PosTot.X2,136+18*(PosTot.Y2+Y));
            AddrWindow->draw(sprite);
            sprite.setPosition(360+18*PosTot.X3,136+18*(PosTot.Y3+Y));
            AddrWindow->draw(sprite);
            sprite.setPosition(360+18*PosTot.X4,136+18*(PosTot.Y4+Y));
            AddrWindow->draw(sprite);   
            break;
        }
        
    }


    
    
}


int bloc::GetY(){
    return PosTot.Y1;
}

void bloc::Recommencer(){
    VPerdu = false;
    score = 0;
    LigneDetruiteTot = 0;
    LigneDetruite = 0;
    LigneComplete = 0;
    Niveau = 0;
    ViderTableau();

    BlocSaved = 0;
    CouleurSaved = 0;
    NbBloc = 0; NbBlocSuivant=0; CouleurAleaSuivant=0; CouleurAlea =0;

    ResetBloc();
}