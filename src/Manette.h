#pragma once

#include <SFML/Window.hpp>
#include <array>
#include <cstdint>
#include <optional>

// Manettes : boutons et directions d'axes deviennent des codes entiers, dans des plages
// distinctes de celles du clavier, pour partager la même table d'actions.
namespace manette {

inline constexpr int BASE_BOUTON = 1000;
inline constexpr int NB_BOUTONS = 32;
inline constexpr int BASE_AXE = 2000;
inline constexpr int NB_AXES = sf::Joystick::AxisCount;

int CodeBouton(unsigned bouton);
int CodeAxe(sf::Joystick::Axis axe, bool positif);
bool EstManette(int code);
bool EstBouton(int code);
// Pour un code d'axe : l'axe et le sens
sf::Joystick::Axis Axe(int code);
bool Positif(int code);

// Vrai si l'entrée est actuellement enfoncée sur une des manettes connectées
bool EstEnfonce(int code);

struct Entree {
    int code;
    bool appui; // false = relâchement
};

// Transforme les événements de manette en appuis/relâchements, avec un seuil et une
// hystérésis sur les axes pour ignorer le bruit des sticks.
class Traducteur {
public:
    // Renvoie le nombre d'entrées écrites (0 à 2 : un axe qui change de sens relâche puis appuie)
    int Traduire(const sf::Event& evenement, std::array<Entree, 2>& sortie);

private:
    std::array<std::array<std::int8_t, NB_AXES>, sf::Joystick::Count> sens{};
};

// Pour les menus : croix/stick → flèches, bouton A → Entrée, B et Start → Échap
std::optional<sf::Event> VersClavier(const Entree& entree);

} // namespace manette
