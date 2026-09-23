#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

namespace ressources {

// Dossier contenant l'exécutable (vide si introuvable)
std::filesystem::path DossierExecutable();

// Cherche asset/<nom> à côté de l'exécutable, puis dans son dossier parent (jamais le dossier courant).
// Affiche les dossiers essayés sur la sortie d'erreur si le fichier est introuvable.
std::optional<std::filesystem::path> Trouver(std::string_view nom);

// Charge une ressource SFML (texture, police...) depuis le dossier asset
template <typename T>
bool Charger(T& ressource, std::string_view nom) {
    const auto chemin = Trouver(nom);
    if (!chemin) return false;
    // Une police est lue au fur et à mesure depuis son fichier : SFML « l'ouvre » au lieu de la charger
    if constexpr (requires { ressource.openFromFile(*chemin); })
        return ressource.openFromFile(*chemin);
    else
        return ressource.loadFromFile(*chemin);
}

} // namespace ressources
