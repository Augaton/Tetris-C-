#pragma once

#include "Mode.h"

#include <array>
#include <filesystem>
#include <string>
#include <vector>

struct EntreeClassement {
    long long score = 0;
    int lignes = 0;
    int tempsMs = 0;  // durée de la partie
    std::string date; // « AAAA-MM-JJ HH:MM », vide si inconnue
};

// Top 10 local de chaque mode. Sprint : temps le plus court ; autres modes : meilleur score.
class Classements {
public:
    static constexpr size_t TAILLE = 10;

    // Ajoute l'entrée à sa place. Renvoie son rang (0 = premier) ou -1 si elle n'entre pas.
    int Ajouter(Mode mode, const EntreeClassement& entree);

    const std::vector<EntreeClassement>& Table(Mode mode) const { return tables[static_cast<size_t>(mode)]; }
    const EntreeClassement* Premier(Mode mode) const;

    // Vrai si `a` est mieux classée que `b`
    static bool Meilleure(Mode mode, const EntreeClassement& a, const EntreeClassement& b);

    std::string Serialiser() const;
    // Lignes invalides ignorées, tables retriées et tronquées
    static Classements Analyser(const std::string& contenu);

private:
    std::array<std::vector<EntreeClassement>, NB_MODES> tables;
};

namespace classements {

std::filesystem::path CheminFichier();
Classements Charger(const std::filesystem::path& chemin);
bool Sauvegarder(const std::filesystem::path& chemin, const Classements& table);

// Date et heure locales au format du classement
std::string DateActuelle();

} // namespace classements
