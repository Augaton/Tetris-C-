#include "Touches.h"

#include "Manette.h"
#include "Texte.h"

#include <cstdlib>
#include <vector>

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#endif

namespace {

using K = sf::Keyboard;

struct InfoTouche {
    K::Key code;
    std::string identifiant;
    std::string nom;       // UTF-8, français
    std::string nomAnglais; // vide = identique
};

const std::vector<InfoTouche>& Table() {
    static const std::vector<InfoTouche> table = [] {
        std::vector<InfoTouche> t;
        for (int i = 0; i < 26; i++) {
            const std::string lettre(1, static_cast<char>('A' + i));
            t.push_back({static_cast<K::Key>(K::A + i), lettre, lettre, ""});
        }
        for (int i = 0; i < 10; i++) {
            const std::string chiffre = std::to_string(i);
            t.push_back({static_cast<K::Key>(K::Num0 + i), "Num" + chiffre, chiffre, ""});
            t.push_back({static_cast<K::Key>(K::Numpad0 + i), "Numpad" + chiffre, "Pavé " + chiffre, "Num " + chiffre});
        }
        for (int i = 0; i < 15; i++) {
            const std::string f = "F" + std::to_string(i + 1);
            t.push_back({static_cast<K::Key>(K::F1 + i), f, f, ""});
        }
        const std::vector<InfoTouche> autres = {
            {K::Escape, "Escape", "Échap", "Esc"},          {K::LControl, "LControl", "Ctrl G", "L Ctrl"},
            {K::LShift, "LShift", "Maj G", "L Shift"},        {K::LAlt, "LAlt", "Alt", ""},
            {K::LSystem, "LSystem", "Système G", "L Super"},  {K::RControl, "RControl", "Ctrl D", "R Ctrl"},
            {K::RShift, "RShift", "Maj D", "R Shift"},        {K::RAlt, "RAlt", "Alt Gr", ""},
            {K::RSystem, "RSystem", "Système D", "R Super"},  {K::Menu, "Menu", "Menu", ""},
            {K::LBracket, "LBracket", "[", ""},                {K::RBracket, "RBracket", "]", ""},
            {K::Semicolon, "Semicolon", ";", ""},              {K::Comma, "Comma", ",", ""},
            {K::Period, "Period", ".", ""},                    {K::Apostrophe, "Apostrophe", "'", ""},
            {K::Slash, "Slash", "/", ""},                      {K::Backslash, "Backslash", "\\", ""},
            {K::Grave, "Grave", "`", ""},                      {K::Equal, "Equal", "=", ""},
            {K::Hyphen, "Hyphen", "-", ""},                    {K::Space, "Space", "Espace", "Space"},
            {K::Enter, "Enter", "Entrée", "Enter"},            {K::Backspace, "Backspace", "Retour arr.", "Backspace"},
            {K::Tab, "Tab", "Tab", ""},                        {K::PageUp, "PageUp", "Page préc.", "Page Up"},
            {K::PageDown, "PageDown", "Page suiv.", "Page Down"}, {K::End, "End", "Fin", "End"},
            {K::Home, "Home", "Début", "Home"},                {K::Insert, "Insert", "Inser", "Insert"},
            {K::Delete, "Delete", "Suppr", "Delete"},          {K::Add, "Add", "Pavé +", "Num +"},
            {K::Subtract, "Subtract", "Pavé -", "Num -"},      {K::Multiply, "Multiply", "Pavé *", "Num *"},
            {K::Divide, "Divide", "Pavé /", "Num /"},          {K::Left, "Left", "←", ""},
            {K::Right, "Right", "→", ""},                      {K::Up, "Up", "↑", ""},
            {K::Down, "Down", "↓", ""},                        {K::Pause, "Pause", "Pause", ""},
        };
        t.insert(t.end(), autres.begin(), autres.end());
        return t;
    }();
    return table;
}

const InfoTouche* Chercher(int code) {
    for (const InfoTouche& info : Table())
        if (info.code == code) return &info;
    return nullptr;
}

// Nom des axes pour le fichier : « AxeX+ », « AxePovY- »...
const char* NOMS_AXES[] = {"X", "Y", "Z", "R", "U", "V", "PovX", "PovY"};

std::string NomEntreeManette(int code) {
    if (manette::EstBouton(code))
        return std::string(Tr("Bouton ", "Button ")) + std::to_string(code - manette::BASE_BOUTON + 1);

    const auto axe = manette::Axe(code);
    const bool positif = manette::Positif(code);
    switch (axe) {
        case sf::Joystick::X:    return std::string("Stick ") + (positif ? "→" : "←");
        case sf::Joystick::Y:    return std::string("Stick ") + (positif ? "↓" : "↑");
        case sf::Joystick::PovX: return std::string(Tr("Croix ", "D-pad ")) + (positif ? "→" : "←");
        case sf::Joystick::PovY: return std::string(Tr("Croix ", "D-pad ")) + (positif ? "↑" : "↓");
        default:
            return std::string(Tr("Axe ", "Axis ")) + NOMS_AXES[static_cast<size_t>(axe)] + (positif ? "+" : "-");
    }
}

// Langue du système : anglais si elle est connue et n'est pas le français
Langue LangueSysteme() {
#ifdef _WIN32
    return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_FRENCH ? Langue::Francais : Langue::Anglais;
#else
    for (const char* variable : {"LC_ALL", "LC_MESSAGES", "LANG"}) {
        const char* valeur = std::getenv(variable);
        if (valeur && *valeur && std::string(valeur) != "C" && std::string(valeur) != "POSIX")
            return std::string(valeur).rfind("fr", 0) == 0 ? Langue::Francais : Langue::Anglais;
    }
    return Langue::Francais;
#endif
}

} // namespace

namespace touches {

std::optional<int> Code(const std::string& identifiant) {
    for (const InfoTouche& info : Table())
        if (info.identifiant == identifiant) return info.code;

    if (identifiant.rfind("Bouton", 0) == 0) {
        const std::string numero = identifiant.substr(6);
        if (numero.empty() || numero.size() > 2 || numero.find_first_not_of("0123456789") != std::string::npos)
            return std::nullopt;
        const int bouton = std::stoi(numero);
        if (bouton < manette::NB_BOUTONS) return manette::CodeBouton(static_cast<unsigned>(bouton));
        return std::nullopt;
    }
    if (identifiant.rfind("Axe", 0) == 0 && identifiant.size() > 4) {
        const char signe = identifiant.back();
        const std::string nom = identifiant.substr(3, identifiant.size() - 4);
        if (signe != '+' && signe != '-') return std::nullopt;
        for (int a = 0; a < manette::NB_AXES; a++)
            if (nom == NOMS_AXES[a]) return manette::CodeAxe(static_cast<sf::Joystick::Axis>(a), signe == '+');
    }
    return std::nullopt;
}

std::string Identifiant(int code) {
    if (manette::EstBouton(code)) return "Bouton" + std::to_string(code - manette::BASE_BOUTON);
    if (manette::EstManette(code))
        return std::string("Axe") + NOMS_AXES[static_cast<size_t>(manette::Axe(code))] + (manette::Positif(code) ? "+" : "-");
    const InfoTouche* info = Chercher(code);
    return info ? info->identifiant : std::string{};
}

sf::String Nom(int code) {
    if (code == AUCUNE_TOUCHE) return Utf8("—");
    if (manette::EstManette(code)) return Utf8(NomEntreeManette(code));
    const InfoTouche* info = Chercher(code);
    if (!info) return Utf8("?");
    return Utf8(LangueActuelle() == Langue::Anglais && !info->nomAnglais.empty() ? info->nomAnglais : info->nom);
}

sf::String NomAction(Action action) {
    switch (action) {
        case Action::Gauche:             return TrU("Gauche", "Left");
        case Action::Droite:             return TrU("Droite", "Right");
        case Action::DescenteDouce:      return TrU("Descente rapide", "Soft drop");
        case Action::ChuteRapide:        return TrU("Chute directe", "Hard drop");
        case Action::TournerHoraire:     return TrU("Tourner (horaire)", "Rotate clockwise");
        case Action::TournerAntiHoraire: return TrU("Tourner (anti-horaire)", "Rotate counterclockwise");
        case Action::Garder:             return TrU("Garder la pièce", "Hold");
        case Action::Pause:              return TrU("Pause", "Pause");
        case Action::Abandonner:         return TrU("Abandonner", "Quit game");
    }
    return {};
}

bool Attribuable(int code) {
    return code != K::Unknown && code != K::F11 && Chercher(code) != nullptr;
}

Reglages ParDefaut() {
    using J = sf::Joystick;
    Reglages r;
    r.langue = LangueSysteme();

    // Secondaires d'abord : la dernière touche assignée devient la principale
    r.AssignerTouche(Action::Gauche, K::Left);
    r.AssignerTouche(Action::Droite, K::Right);
    r.AssignerTouche(Action::DescenteDouce, K::Down);
    r.AssignerTouche(Action::ChuteRapide, K::Space);
    r.AssignerTouche(Action::TournerHoraire, K::Up);
    r.AssignerTouche(Action::TournerHoraire, K::Enter);
    r.AssignerTouche(Action::TournerAntiHoraire, K::RControl);
    r.AssignerTouche(Action::Garder, K::C);
    r.AssignerTouche(Action::Garder, K::RShift);
    r.AssignerTouche(Action::Pause, K::Escape);
    r.AssignerTouche(Action::Pause, K::P);
    r.AssignerTouche(Action::Abandonner, K::A);

    // Disposition type Xbox : A=0, B=1, X=2, Y=3, LB=4, RB=5, Back=6, Start=7
    r.AssignerBouton(Action::Gauche, manette::CodeAxe(J::X, false));
    r.AssignerBouton(Action::Gauche, manette::CodeAxe(J::PovX, false));
    r.AssignerBouton(Action::Droite, manette::CodeAxe(J::X, true));
    r.AssignerBouton(Action::Droite, manette::CodeAxe(J::PovX, true));
    r.AssignerBouton(Action::DescenteDouce, manette::CodeAxe(J::Y, true));
    r.AssignerBouton(Action::DescenteDouce, manette::CodeAxe(J::PovY, false));
    r.AssignerBouton(Action::ChuteRapide, manette::CodeAxe(J::PovY, true));
    r.AssignerBouton(Action::TournerHoraire, manette::CodeBouton(3));
    r.AssignerBouton(Action::TournerHoraire, manette::CodeBouton(0));
    r.AssignerBouton(Action::TournerAntiHoraire, manette::CodeBouton(2));
    r.AssignerBouton(Action::TournerAntiHoraire, manette::CodeBouton(1));
    r.AssignerBouton(Action::Garder, manette::CodeBouton(5));
    r.AssignerBouton(Action::Garder, manette::CodeBouton(4));
    r.AssignerBouton(Action::Pause, manette::CodeBouton(7));
    r.AssignerBouton(Action::Abandonner, manette::CodeBouton(6));
    return r;
}

} // namespace touches
