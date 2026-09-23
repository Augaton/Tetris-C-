#include "Touches.h"

#include "Manette.h"
#include "Texte.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdlib>
#include <format>
#include <vector>

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#endif

namespace {

using Touche = sf::Keyboard::Key;

struct InfoTouche {
    Touche touche;
    std::string identifiant;
    std::string nom;        // UTF-8, français
    std::string nomAnglais; // vide = identique
};

const std::vector<InfoTouche>& Table() {
    static const std::vector<InfoTouche> table = [] {
        // Lettres, chiffres et touches de fonction se suivent dans l'énumération de SFML
        const auto decaler = [](Touche premiere, int i) { return static_cast<Touche>(static_cast<int>(premiere) + i); };
        std::vector<InfoTouche> t;
        for (int i = 0; i < 26; i++) {
            const std::string lettre(1, static_cast<char>('A' + i));
            t.push_back({decaler(Touche::A, i), lettre, lettre, ""});
        }
        for (int i = 0; i < 10; i++) {
            const std::string chiffre = std::to_string(i);
            t.push_back({decaler(Touche::Num0, i), "Num" + chiffre, chiffre, ""});
            t.push_back({decaler(Touche::Numpad0, i), "Numpad" + chiffre, "Pavé " + chiffre, "Num " + chiffre});
        }
        for (int i = 0; i < 15; i++) {
            const std::string f = "F" + std::to_string(i + 1);
            t.push_back({decaler(Touche::F1, i), f, f, ""});
        }
        const std::vector<InfoTouche> autres = {
            {Touche::Escape, "Escape", "Échap", "Esc"},           {Touche::LControl, "LControl", "Ctrl G", "L Ctrl"},
            {Touche::LShift, "LShift", "Maj G", "L Shift"},       {Touche::LAlt, "LAlt", "Alt", ""},
            {Touche::LSystem, "LSystem", "Système G", "L Super"}, {Touche::RControl, "RControl", "Ctrl D", "R Ctrl"},
            {Touche::RShift, "RShift", "Maj D", "R Shift"},       {Touche::RAlt, "RAlt", "Alt Gr", ""},
            {Touche::RSystem, "RSystem", "Système D", "R Super"}, {Touche::Menu, "Menu", "Menu", ""},
            {Touche::LBracket, "LBracket", "[", ""},              {Touche::RBracket, "RBracket", "]", ""},
            {Touche::Semicolon, "Semicolon", ";", ""},            {Touche::Comma, "Comma", ",", ""},
            {Touche::Period, "Period", ".", ""},                  {Touche::Apostrophe, "Apostrophe", "'", ""},
            {Touche::Slash, "Slash", "/", ""},                    {Touche::Backslash, "Backslash", "\\", ""},
            {Touche::Grave, "Grave", "`", ""},                    {Touche::Equal, "Equal", "=", ""},
            {Touche::Hyphen, "Hyphen", "-", ""},                  {Touche::Space, "Space", "Espace", "Space"},
            {Touche::Enter, "Enter", "Entrée", "Enter"},          {Touche::Backspace, "Backspace", "Retour arr.", "Backspace"},
            {Touche::Tab, "Tab", "Tab", ""},                      {Touche::PageUp, "PageUp", "Page préc.", "Page Up"},
            {Touche::PageDown, "PageDown", "Page suiv.", "Page Down"}, {Touche::End, "End", "Fin", "End"},
            {Touche::Home, "Home", "Début", "Home"},              {Touche::Insert, "Insert", "Inser", "Insert"},
            {Touche::Delete, "Delete", "Suppr", "Delete"},        {Touche::Add, "Add", "Pavé +", "Num +"},
            {Touche::Subtract, "Subtract", "Pavé -", "Num -"},    {Touche::Multiply, "Multiply", "Pavé *", "Num *"},
            {Touche::Divide, "Divide", "Pavé /", "Num /"},        {Touche::Left, "Left", "←", ""},
            {Touche::Right, "Right", "→", ""},                    {Touche::Up, "Up", "↑", ""},
            {Touche::Down, "Down", "↓", ""},                      {Touche::Pause, "Pause", "Pause", ""},
        };
        t.insert(t.end(), autres.begin(), autres.end());
        return t;
    }();
    return table;
}

const InfoTouche* Chercher(int code) {
    const auto codeDe = [](const InfoTouche& info) { return touches::CodeTouche(info.touche); };
    const auto trouvee = std::ranges::find(Table(), code, codeDe);
    return trouvee == Table().end() ? nullptr : &*trouvee;
}

// Nom des axes pour le fichier : « AxeX+ », « AxePovY- »...
constexpr std::array<std::string_view, manette::NB_AXES> NOMS_AXES = {"X", "Y", "Z", "R", "U", "V", "PovX", "PovY"};

std::string_view NomAxe(int code) {
    return NOMS_AXES[static_cast<size_t>(manette::Axe(code))];
}

std::string NomEntreeManette(int code) {
    if (manette::EstBouton(code)) return std::format("{}{}", Tr("Bouton ", "Button "), code - manette::BASE_BOUTON + 1);

    const bool positif = manette::Positif(code);
    switch (manette::Axe(code)) {
        case sf::Joystick::Axis::X:    return std::format("Stick {}", positif ? "→" : "←");
        case sf::Joystick::Axis::Y:    return std::format("Stick {}", positif ? "↓" : "↑");
        case sf::Joystick::Axis::PovX: return std::format("{}{}", Tr("Croix ", "D-pad "), positif ? "→" : "←");
        case sf::Joystick::Axis::PovY: return std::format("{}{}", Tr("Croix ", "D-pad "), positif ? "↑" : "↓");
        default:                       return std::format("{}{}{}", Tr("Axe ", "Axis "), NomAxe(code), positif ? '+' : '-');
    }
}

// Langue du système : anglais si elle est connue et n'est pas le français
Langue LangueSysteme() {
#ifdef _WIN32
    return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_FRENCH ? Langue::Francais : Langue::Anglais;
#else
    for (const char* variable : {"LC_ALL", "LC_MESSAGES", "LANG"}) {
        const char* brute = std::getenv(variable);
        const std::string_view valeur = brute ? brute : "";
        if (!valeur.empty() && valeur != "C" && valeur != "POSIX")
            return valeur.starts_with("fr") ? Langue::Francais : Langue::Anglais;
    }
    return Langue::Francais;
#endif
}

} // namespace

namespace touches {

std::optional<int> Code(std::string_view identifiant) {
    for (const InfoTouche& info : Table())
        if (info.identifiant == identifiant) return CodeTouche(info.touche);

    if (identifiant.starts_with("Bouton")) {
        // Un ou deux chiffres, sans signe
        const std::string_view numero = identifiant.substr(6);
        if (numero.empty() || numero.size() > 2 || numero.find_first_not_of("0123456789") != std::string_view::npos)
            return std::nullopt;
        int bouton = 0;
        const auto resultat = std::from_chars(numero.data(), numero.data() + numero.size(), bouton);
        if (resultat.ec != std::errc{} || bouton >= manette::NB_BOUTONS) return std::nullopt;
        return manette::CodeBouton(static_cast<unsigned>(bouton));
    }
    if (identifiant.starts_with("Axe") && identifiant.size() > 4) {
        const char signe = identifiant.back();
        if (signe != '+' && signe != '-') return std::nullopt;
        const auto axe = std::ranges::find(NOMS_AXES, identifiant.substr(3, identifiant.size() - 4));
        if (axe != NOMS_AXES.end())
            return manette::CodeAxe(static_cast<sf::Joystick::Axis>(axe - NOMS_AXES.begin()), signe == '+');
    }
    return std::nullopt;
}

std::string Identifiant(int code) {
    if (manette::EstBouton(code)) return std::format("Bouton{}", code - manette::BASE_BOUTON);
    if (manette::EstManette(code)) return std::format("Axe{}{}", NomAxe(code), manette::Positif(code) ? '+' : '-');
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

bool Attribuable(sf::Keyboard::Key touche) {
    return touche != Touche::Unknown && touche != Touche::F11 && Chercher(CodeTouche(touche)) != nullptr;
}

Reglages ParDefaut() {
    using J = sf::Joystick::Axis;
    Reglages r;
    r.langue = LangueSysteme();

    // Secondaires d'abord : la dernière touche assignée devient la principale
    const auto assigner = [&r](Action action, Touche touche) { r.AssignerTouche(action, CodeTouche(touche)); };
    assigner(Action::Gauche, Touche::Left);
    assigner(Action::Droite, Touche::Right);
    assigner(Action::DescenteDouce, Touche::Down);
    assigner(Action::ChuteRapide, Touche::Space);
    assigner(Action::TournerHoraire, Touche::Up);
    assigner(Action::TournerHoraire, Touche::Enter);
    assigner(Action::TournerAntiHoraire, Touche::RControl);
    assigner(Action::Garder, Touche::C);
    assigner(Action::Garder, Touche::RShift);
    assigner(Action::Pause, Touche::Escape);
    assigner(Action::Pause, Touche::P);
    assigner(Action::Abandonner, Touche::A);

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
