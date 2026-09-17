#pragma once

#include "Reglages.h"

#include <SFML/Graphics.hpp>
#include <optional>
#include <string>

namespace touches {

// Nom utilisé dans le fichier de réglages ("Left", "A", "Numpad0"...)
std::optional<int> Code(const std::string& identifiant);
std::string Identifiant(int code);

// Nom affiché à l'écran ("←", "Entrée", "Ctrl D"...), "—" si aucune touche
sf::String Nom(int code);

sf::String NomAction(Action action);

// Touches qu'on peut attribuer (F11 est réservée au plein écran)
bool Attribuable(int code);

Reglages ParDefaut();

} // namespace touches
