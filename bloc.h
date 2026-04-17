#include <SFML/Graphics.hpp>
#include <string>

class bloc{

    private:
        sf::RenderWindow* AddrWindow;
        sf::Sprite Tiles;
        sf::Vector2f position;

        int score =0, Niveau=0, LigneDetruite=0, LigneDetruiteTot=0, Vitesse=0,rotation=0, BlocSaved = 8, CouleurSaved;
        int Patterne[28], map[20][10], PatterneApp[28], TabX, TabY, Rotation, CouleurAlea, CouleurAleaSuivant, LigneComplete, NbBloc, NbBlocSuivant;
        struct LesPos{
            int X1,X2,X3,X4,Y1,Y2,Y3,Y4;
        };
        LesPos PosTot;
        bool VPerdu= false, DejaSave=false;
        int combo = 0;

        void DeplacementGauche();
        void DeplacementDroite();
        void DeplacementBas();
        void DrawTiles();

    public:
        bloc(const sf::Texture &Textruc ,sf::RenderWindow* window, int initialX, int initialY);
        ~bloc();
        void assembly();
        void next();
        void mouvement(std::string NomMouv);
        void ScoreAdd(std::string TypePts, int Nbr);
        void drawASprite(sf::Sprite &Tile);
        void DessinerLeTableau();
        void Ajouter(int X, int Y,int Nbr);
        void InitialiserPOS();
        void RegenererBloc();
        void CouleurAleatoire();
        void BlocAleatoire();
        void ResetBloc();
        void VoirLeTableau();
        void ViderTableau();
        void DefinitionDeStruct(int Plage, int Pattern, int i, int j);
        void ChangementNiveau();
        void RotationBloc();
        void ChangerBloc();
        void RemplacerBlocSave();
        void Saved();
        void VisualiserBloc();
        void AtterirEnBas();
        void Recommencer();


        int GetY();
        int ClearLines();

        bool checkLine();
        bool checkmove(int x, int y);
        bool DetectionBlocEnBas();
        bool Perdu();
        bool DetectionBlocEmpile();


        inline void Effacer(){ AddrWindow->clear();};
        inline void Dessiner(){ AddrWindow->display();};
        inline int AfficherBlocSuivant(){ return NbBlocSuivant; };
        inline int AfficherBlocSaved(){ return BlocSaved;};
        inline int VitesseBloc(){ return ( (60*100)-(2*Niveau*100))/4   ;};
        inline std::string AfficherLigneDetruite(){ return std::to_string(LigneDetruiteTot);};
        inline std::string AfficherNiveau(){return std::to_string(Niveau);};
        inline std::string AfficherScore(){return std::to_string(score);};
        inline sf::Vector2f RevoyerPosition() {return Tiles.getPosition(); };

        inline void ResetCombo(){ combo = 0; }
        inline int AfficherCombo(){ return combo; }
};