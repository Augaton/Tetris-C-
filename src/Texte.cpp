#include "Texte.h"

#include <algorithm>
#include <cmath>

sf::String Utf8(const std::string& texte) {
    return sf::String::fromUtf8(texte.begin(), texte.end());
}

void PlacerTexte(sf::Text& texte, unsigned tailleLogique, sf::Vector2f centre, float echelle, float zoom) {
    const unsigned tailleReelle =
        std::max(1u, static_cast<unsigned>(std::lround(static_cast<float>(tailleLogique) * echelle)));
    texte.setCharacterSize(tailleReelle);

    const float facteur = zoom * static_cast<float>(tailleLogique) / static_cast<float>(tailleReelle);
    texte.setScale(facteur, facteur);

    const sf::FloatRect b = texte.getLocalBounds();
    texte.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
    texte.setPosition(centre);
}

void PlacerTexteBorne(sf::Text& texte, unsigned tailleLogique, sf::Vector2f centre, float echelle, float largeurMax) {
    PlacerTexte(texte, tailleLogique, centre, echelle);
    const float largeur = texte.getGlobalBounds().width;
    if (largeur > largeurMax) PlacerTexte(texte, tailleLogique, centre, echelle, largeurMax / largeur);
}
