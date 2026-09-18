#include "Texte.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

Langue langueCourante = Langue::Francais;

} // namespace

sf::String Utf8(const std::string& texte) {
    return sf::String::fromUtf8(texte.begin(), texte.end());
}

void DefinirLangue(Langue langue) {
    langueCourante = langue;
}

Langue LangueActuelle() {
    return langueCourante;
}

const char* Tr(const char* francais, const char* anglais) {
    return langueCourante == Langue::Anglais ? anglais : francais;
}

sf::String Majuscules(const sf::String& texte) {
    sf::String resultat = texte;
    for (std::size_t i = 0; i < resultat.getSize(); i++) {
        sf::Uint32 c = resultat[i];
        if (c >= 'a' && c <= 'z') c -= 0x20;
        else if (c >= 0xE0 && c <= 0xFE && c != 0xF7) c -= 0x20; // à…þ sauf ÷
        else if (c == 0xFF) c = 0x178;                           // ÿ
        else if (c == 0x153) c = 0x152;                          // œ
        resultat[i] = c;
    }
    return resultat;
}

std::string FormaterTemps(float secondes) {
    // Borné à 99:59.99 : pas de dépassement, même après une session Zen interminable
    const long centiemes = std::lround(std::clamp(secondes, 0.f, 5999.99f) * 100.f);
    char tampon[32];
    std::snprintf(tampon, sizeof tampon, "%ld:%02ld.%02ld", centiemes / 6000, (centiemes / 100) % 60, centiemes % 100);
    return tampon;
}

std::string FormaterNombre(long long nombre) {
    // Valeur absolue en non signé : pas de dépassement pour le plus petit long long
    const unsigned long long absolu = nombre < 0 ? 0ULL - static_cast<unsigned long long>(nombre)
                                                 : static_cast<unsigned long long>(nombre);
    std::string chiffres = std::to_string(absolu);
    // Espace insécable en français (présente dans toutes les polices), virgule en anglais
    const std::string separateur = langueCourante == Langue::Anglais ? "," : "\xC2\xA0";
    for (int i = static_cast<int>(chiffres.size()) - 3; i > 0; i -= 3) chiffres.insert(static_cast<size_t>(i), separateur);
    return nombre < 0 ? "-" + chiffres : chiffres;
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
