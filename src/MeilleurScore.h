#pragma once

#include <filesystem>

namespace meilleur_score {

// meilleur_score.txt dans le dossier des données du jeu (voir fichiers::DossierDonnees)
std::filesystem::path CheminFichier();

// Renvoie 0 si le fichier est absent ou invalide
long long Charger(const std::filesystem::path& chemin);

bool Sauvegarder(const std::filesystem::path& chemin, long long score);

} // namespace meilleur_score
