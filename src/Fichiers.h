#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

namespace fichiers {

// Dossier des données du jeu (TetrisC) :
// Linux $XDG_DATA_HOME ou ~/.local/share, Windows %APPDATA%, macOS ~/Library/Application Support.
// Vide si aucun n'est disponible (on utilise alors le dossier courant).
std::filesystem::path DossierDonnees();

// Lit un petit fichier texte. nullopt s'il est absent, illisible ou plus grand que `tailleMax`.
std::optional<std::string> LireTout(const std::filesystem::path& chemin, std::size_t tailleMax = 64 * 1024);

// Écrit dans un fichier temporaire, le synchronise sur le disque puis le renomme :
// ni fichier à moitié écrit, ni fichier vide après une coupure de courant
bool EcrireAtomique(const std::filesystem::path& chemin, const std::string& contenu);

} // namespace fichiers
