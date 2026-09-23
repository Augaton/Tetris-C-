#include "MeilleurScore.h"

#include "Fichiers.h"

#include <cctype>
#include <charconv>
#include <string>
#include <string_view>
#include <system_error>

namespace fs = std::filesystem;

namespace meilleur_score {

fs::path CheminFichier() {
    return fichiers::DossierDonnees() / "meilleur_score.txt";
}

long long Charger(const fs::path& chemin) {
    const auto contenu = fichiers::LireTout(chemin);
    if (!contenu) return 0;

    std::string_view ligne(*contenu);
    ligne = ligne.substr(0, ligne.find('\n'));
    while (!ligne.empty() && std::isspace(static_cast<unsigned char>(ligne.back()))) ligne.remove_suffix(1);

    long long valeur = 0;
    const char* fin = ligne.data() + ligne.size();
    const auto [ptr, erreur] = std::from_chars(ligne.data(), fin, valeur);
    if (erreur != std::errc{} || ptr != fin || valeur < 0) return 0;
    return valeur;
}

bool Sauvegarder(const fs::path& chemin, long long score) {
    return fichiers::EcrireAtomique(chemin, std::to_string(score) + '\n');
}

} // namespace meilleur_score
