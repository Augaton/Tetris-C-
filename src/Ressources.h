#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace ressources {

// Dossier contenant l'exécutable (vide si introuvable)
std::filesystem::path DossierExecutable();

// Cherche asset/<nom> à côté de l'exécutable, puis dans son dossier parent (jamais le dossier courant).
// Affiche les dossiers essayés sur la sortie d'erreur si le fichier est introuvable.
std::optional<std::filesystem::path> Trouver(const std::string& nom);

// Charge une ressource SFML (texture, police...) depuis le dossier asset
template <typename T>
bool Charger(T& ressource, const std::string& nom) {
    const auto chemin = Trouver(nom);
    return chemin && ressource.loadFromFile(chemin->string());
}

} // namespace ressources
