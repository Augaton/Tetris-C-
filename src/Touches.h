#pragma once

#include "Reglages.h"

#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <string_view>

// Noms des touches du clavier et des entrées de manette (codes de Reglages)
namespace touches {

// Code d'une touche du clavier dans les réglages (les entrées de manette ont leurs propres plages)
constexpr int CodeTouche(sf::Keyboard::Key touche) { return static_cast<int>(touche); }

// Nom utilisé dans le fichier de réglages ("Left", "A", "Bouton0", "AxePovX-"...)
std::optional<int> Code(std::string_view identifiant);
std::string Identifiant(int code);

// Nom affiché dans la langue courante ("←", "Entrée", "Croix ↑", "Bouton 1"...), "—" si aucune
sf::String Nom(int code);

sf::String NomAction(Action action);

// Touches du clavier qu'on peut attribuer (F11 est réservée au plein écran)
bool Attribuable(sf::Keyboard::Key touche);

// Touches, boutons par défaut et langue du système
Reglages ParDefaut();

} // namespace touches
