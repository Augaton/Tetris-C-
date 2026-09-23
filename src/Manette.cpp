#include "Manette.h"

#include <cstdlib>

namespace {

constexpr float SEUIL_APPUI = 50.f;
constexpr float SEUIL_RELACHE = 30.f;

std::int8_t Sens(float position, std::int8_t precedent) {
    if (position > SEUIL_APPUI) return 1;
    if (position < -SEUIL_APPUI) return -1;
    if (std::abs(position) < SEUIL_RELACHE) return 0;
    return precedent; // zone intermédiaire : on garde l'état (hystérésis)
}

} // namespace

namespace manette {

int CodeBouton(unsigned bouton) {
    return BASE_BOUTON + static_cast<int>(bouton);
}

int CodeAxe(sf::Joystick::Axis axe, bool positif) {
    return BASE_AXE + static_cast<int>(axe) * 2 + (positif ? 1 : 0);
}

bool EstBouton(int code) {
    return code >= BASE_BOUTON && code < BASE_BOUTON + NB_BOUTONS;
}

bool EstManette(int code) {
    return EstBouton(code) || (code >= BASE_AXE && code < BASE_AXE + NB_AXES * 2);
}

sf::Joystick::Axis Axe(int code) {
    return static_cast<sf::Joystick::Axis>((code - BASE_AXE) / 2);
}

bool Positif(int code) {
    return (code - BASE_AXE) % 2 == 1;
}

bool EstEnfonce(int code) {
    if (!EstManette(code)) return false;
    for (unsigned j = 0; j < sf::Joystick::Count; j++) {
        if (!sf::Joystick::isConnected(j)) continue;
        if (EstBouton(code)) {
            if (sf::Joystick::isButtonPressed(j, static_cast<unsigned>(code - BASE_BOUTON))) return true;
        } else if (sf::Joystick::hasAxis(j, Axe(code))) {
            const float position = sf::Joystick::getAxisPosition(j, Axe(code));
            if (Positif(code) ? position > SEUIL_APPUI : position < -SEUIL_APPUI) return true;
        }
    }
    return false;
}

Entrees Traducteur::Traduire(const sf::Event& evenement) {
    Entrees entrees;
    if (const auto* appui = evenement.getIf<sf::Event::JoystickButtonPressed>()) {
        if (appui->button < NB_BOUTONS) entrees.Ajouter({CodeBouton(appui->button), true});
    } else if (const auto* relache = evenement.getIf<sf::Event::JoystickButtonReleased>()) {
        if (relache->button < NB_BOUTONS) entrees.Ajouter({CodeBouton(relache->button), false});
    } else if (const auto* mouvement = evenement.getIf<sf::Event::JoystickMoved>()) {
        const unsigned j = mouvement->joystickId;
        const auto axe = static_cast<std::size_t>(mouvement->axis);
        if (j >= sf::Joystick::Count || axe >= NB_AXES) return entrees;

        std::int8_t& precedent = sens[j][axe];
        const std::int8_t nouveau = Sens(mouvement->position, precedent);
        if (nouveau == precedent) return entrees;

        if (precedent != 0) entrees.Ajouter({CodeAxe(mouvement->axis, precedent > 0), false});
        if (nouveau != 0) entrees.Ajouter({CodeAxe(mouvement->axis, nouveau > 0), true});
        precedent = nouveau;
    } else if (const auto* deconnexion = evenement.getIf<sf::Event::JoystickDisconnected>()) {
        if (deconnexion->joystickId < sf::Joystick::Count) sens[deconnexion->joystickId].fill(0);
    }
    return entrees;
}

std::optional<sf::Event> VersClavier(const Entree& entree) {
    if (!entree.appui) return std::nullopt;

    using Touche = sf::Keyboard::Key;
    Touche touche = Touche::Unknown;
    if (EstBouton(entree.code)) {
        switch (entree.code - BASE_BOUTON) {
            case 0: touche = Touche::Enter; break;  // A
            case 1:                                 // B
            case 7: touche = Touche::Escape; break; // Start
            default: break;
        }
    } else {
        const bool positif = Positif(entree.code);
        switch (Axe(entree.code)) {
            case sf::Joystick::Axis::X:
            case sf::Joystick::Axis::PovX: touche = positif ? Touche::Right : Touche::Left; break;
            case sf::Joystick::Axis::Y:    touche = positif ? Touche::Down : Touche::Up; break;
            case sf::Joystick::Axis::PovY: touche = positif ? Touche::Up : Touche::Down; break; // croix : + = haut
            default: break;
        }
    }
    if (touche == Touche::Unknown) return std::nullopt;
    return sf::Event::KeyPressed{.code = touche, .scancode = sf::Keyboard::Scan::Unknown};
}

} // namespace manette
