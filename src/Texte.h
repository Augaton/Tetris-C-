#pragma once

#include "Reglages.h"

#include <SFML/Graphics.hpp>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

// Convertit une chaîne UTF-8 (accents, flèches) pour SFML
sf::String Utf8(std::string_view texte);

// Langue de l'interface
void DefinirLangue(Langue langue);
Langue LangueActuelle();
// Texte dans la langue courante (les deux versions sont écrites côte à côte dans le code)
const char* Tr(const char* francais, const char* anglais);
inline sf::String TrU(const char* francais, const char* anglais) { return Utf8(Tr(francais, anglais)); }

// Majuscules, lettres accentuées comprises (é → É, œ → Œ)
sf::String Majuscules(const sf::String& texte);

// « 1:23.45 »
std::string FormaterTemps(float secondes);
// « 12 345 » en français, « 12,345 » en anglais
std::string FormaterNombre(long long nombre);

// Rastérise le texte à sa taille réelle à l'écran (net quelle que soit la taille de la fenêtre),
// le ramène à sa taille logique puis le centre sur `centre`.
void PlacerTexte(sf::Text& texte, unsigned tailleLogique, sf::Vector2f centre, float echelle, float zoom = 1.f);

// Comme PlacerTexte, mais réduit le texte s'il dépasse `largeurMax` (unités logiques)
void PlacerTexteBorne(sf::Text& texte, unsigned tailleLogique, sf::Vector2f centre, float echelle, float largeurMax);

// N objets construits avec les mêmes arguments. sf::Text n'a pas de constructeur par défaut :
// les tableaux de textes (ou d'objets qui en contiennent) se remplissent ainsi avec leur police.
template <typename T, std::size_t N, typename... Arguments>
std::array<T, N> Tableau(const Arguments&... arguments) {
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
        return std::array<T, N>{(static_cast<void>(I), T(arguments...))...};
    }(std::make_index_sequence<N>{});
}
