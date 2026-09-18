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

int Traducteur::Traduire(const sf::Event& evenement, std::array<Entree, 2>& sortie) {
    switch (evenement.type) {
        case sf::Event::JoystickButtonPressed:
        case sf::Event::JoystickButtonReleased:
            if (evenement.joystickButton.button >= NB_BOUTONS) return 0;
            sortie[0] = {CodeBouton(evenement.joystickButton.button),
                         evenement.type == sf::Event::JoystickButtonPressed};
            return 1;

        case sf::Event::JoystickMoved: {
            const unsigned j = evenement.joystickMove.joystickId;
            const auto axe = evenement.joystickMove.axis;
            if (j >= sf::Joystick::Count || static_cast<int>(axe) >= NB_AXES) return 0;

            std::int8_t& precedent = sens[j][static_cast<size_t>(axe)];
            const std::int8_t nouveau = Sens(evenement.joystickMove.position, precedent);
            if (nouveau == precedent) return 0;

            int n = 0;
            if (precedent != 0) sortie[n++] = {CodeAxe(axe, precedent > 0), false};
            if (nouveau != 0) sortie[n++] = {CodeAxe(axe, nouveau > 0), true};
            precedent = nouveau;
            return n;
        }

        case sf::Event::JoystickDisconnected:
            if (evenement.joystickConnect.joystickId < sf::Joystick::Count)
                sens[evenement.joystickConnect.joystickId].fill(0);
            return 0;

        default:
            return 0;
    }
}

std::optional<sf::Event> VersClavier(const Entree& entree) {
    if (!entree.appui) return std::nullopt;

    sf::Keyboard::Key touche = sf::Keyboard::Unknown;
    if (EstBouton(entree.code)) {
        switch (entree.code - BASE_BOUTON) {
            case 0: touche = sf::Keyboard::Enter; break;  // A
            case 1:                                        // B
            case 7: touche = sf::Keyboard::Escape; break;  // Start
            default: break;
        }
    } else {
        const auto axe = Axe(entree.code);
        const bool positif = Positif(entree.code);
        if (axe == sf::Joystick::X || axe == sf::Joystick::PovX) touche = positif ? sf::Keyboard::Right : sf::Keyboard::Left;
        else if (axe == sf::Joystick::Y) touche = positif ? sf::Keyboard::Down : sf::Keyboard::Up;
        else if (axe == sf::Joystick::PovY) touche = positif ? sf::Keyboard::Up : sf::Keyboard::Down; // croix : + = haut
    }
    if (touche == sf::Keyboard::Unknown) return std::nullopt;

    sf::Event clavier{};
    clavier.type = sf::Event::KeyPressed;
    clavier.key.code = touche;
    return clavier;
}

} // namespace manette
