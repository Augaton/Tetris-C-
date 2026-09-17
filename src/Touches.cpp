#include "Touches.h"

#include "Texte.h"

#include <vector>

namespace {

using K = sf::Keyboard;

struct InfoTouche {
    K::Key code;
    std::string identifiant;
    std::string nom; // UTF-8
};

const std::vector<InfoTouche>& Table() {
    static const std::vector<InfoTouche> table = [] {
        std::vector<InfoTouche> t;
        for (int i = 0; i < 26; i++) {
            const std::string lettre(1, static_cast<char>('A' + i));
            t.push_back({static_cast<K::Key>(K::A + i), lettre, lettre});
        }
        for (int i = 0; i < 10; i++) {
            const std::string chiffre = std::to_string(i);
            t.push_back({static_cast<K::Key>(K::Num0 + i), "Num" + chiffre, chiffre});
            t.push_back({static_cast<K::Key>(K::Numpad0 + i), "Numpad" + chiffre, "Pavé " + chiffre});
        }
        for (int i = 0; i < 15; i++) {
            const std::string f = "F" + std::to_string(i + 1);
            t.push_back({static_cast<K::Key>(K::F1 + i), f, f});
        }
        const std::vector<InfoTouche> autres = {
            {K::Escape, "Escape", "Échap"},        {K::LControl, "LControl", "Ctrl G"},
            {K::LShift, "LShift", "Maj G"},         {K::LAlt, "LAlt", "Alt"},
            {K::LSystem, "LSystem", "Système G"},   {K::RControl, "RControl", "Ctrl D"},
            {K::RShift, "RShift", "Maj D"},         {K::RAlt, "RAlt", "Alt Gr"},
            {K::RSystem, "RSystem", "Système D"},   {K::Menu, "Menu", "Menu"},
            {K::LBracket, "LBracket", "["},         {K::RBracket, "RBracket", "]"},
            {K::Semicolon, "Semicolon", ";"},       {K::Comma, "Comma", ","},
            {K::Period, "Period", "."},             {K::Apostrophe, "Apostrophe", "'"},
            {K::Slash, "Slash", "/"},               {K::Backslash, "Backslash", "\\"},
            {K::Grave, "Grave", "`"},               {K::Equal, "Equal", "="},
            {K::Hyphen, "Hyphen", "-"},             {K::Space, "Space", "Espace"},
            {K::Enter, "Enter", "Entrée"},          {K::Backspace, "Backspace", "Retour arr."},
            {K::Tab, "Tab", "Tab"},                 {K::PageUp, "PageUp", "Page préc."},
            {K::PageDown, "PageDown", "Page suiv."}, {K::End, "End", "Fin"},
            {K::Home, "Home", "Début"},             {K::Insert, "Insert", "Inser"},
            {K::Delete, "Delete", "Suppr"},         {K::Add, "Add", "Pavé +"},
            {K::Subtract, "Subtract", "Pavé -"},    {K::Multiply, "Multiply", "Pavé *"},
            {K::Divide, "Divide", "Pavé /"},        {K::Left, "Left", "←"},
            {K::Right, "Right", "→"},               {K::Up, "Up", "↑"},
            {K::Down, "Down", "↓"},                 {K::Pause, "Pause", "Pause"},
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

} // namespace

namespace touches {

std::optional<int> Code(const std::string& identifiant) {
    for (const InfoTouche& info : Table())
        if (info.identifiant == identifiant) return info.code;
    return std::nullopt;
}

std::string Identifiant(int code) {
    const InfoTouche* info = Chercher(code);
    return info ? info->identifiant : std::string{};
}

sf::String Nom(int code) {
    if (code == AUCUNE_TOUCHE) return Utf8("—");
    const InfoTouche* info = Chercher(code);
    return info ? Utf8(info->nom) : Utf8("?");
}

sf::String NomAction(Action action) {
    switch (action) {
        case Action::Gauche:             return Utf8("Gauche");
        case Action::Droite:             return Utf8("Droite");
        case Action::DescenteDouce:      return Utf8("Descente rapide");
        case Action::ChuteRapide:        return Utf8("Chute directe");
        case Action::TournerHoraire:     return Utf8("Tourner (horaire)");
        case Action::TournerAntiHoraire: return Utf8("Tourner (anti-horaire)");
        case Action::Garder:             return Utf8("Garder la pièce");
        case Action::Pause:              return Utf8("Pause");
        case Action::Abandonner:         return Utf8("Abandonner");
    }
    return {};
}

bool Attribuable(int code) {
    return code != K::Unknown && code != K::F11 && Chercher(code) != nullptr;
}

Reglages ParDefaut() {
    Reglages r;
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
    return r;
}

} // namespace touches
