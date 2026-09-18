#include "Classements.h"

#include "Fichiers.h"

#include <algorithm>
#include <charconv>
#include <ctime>
#include <sstream>

namespace {

template <typename T>
bool Nombre(const std::string& texte, T& valeur) {
    const char* fin = texte.data() + texte.size();
    auto [ptr, erreur] = std::from_chars(texte.data(), fin, valeur);
    return erreur == std::errc{} && ptr == fin;
}

// Date venue du fichier : seulement chiffres, espaces, « - » et « : », 16 caractères au plus
bool DateValide(const std::string& date) {
    return date.size() <= 16 && std::all_of(date.begin(), date.end(), [](char c) {
               return (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == ':';
           });
}

} // namespace

bool Classements::Meilleure(Mode mode, const EntreeClassement& a, const EntreeClassement& b) {
    if (mode == Mode::Sprint) return a.tempsMs < b.tempsMs;
    if (a.score != b.score) return a.score > b.score;
    return a.lignes > b.lignes;
}

int Classements::Ajouter(Mode mode, const EntreeClassement& entree) {
    auto& table = tables[static_cast<size_t>(mode)];
    // Après les ex æquo déjà présents : à égalité, le plus ancien reste devant
    const auto position = std::upper_bound(table.begin(), table.end(), entree,
                                           [mode](const auto& a, const auto& b) { return Meilleure(mode, a, b); });
    const auto rang = static_cast<size_t>(position - table.begin());
    if (rang >= TAILLE) return -1;

    table.insert(position, entree);
    if (table.size() > TAILLE) table.resize(TAILLE);
    return static_cast<int>(rang);
}

const EntreeClassement* Classements::Premier(Mode mode) const {
    const auto& table = Table(mode);
    return table.empty() ? nullptr : &table.front();
}

std::string Classements::Serialiser() const {
    std::ostringstream sortie;
    sortie << "# Classements de Tetris : mode;score;lignes;temps_ms;date\n";
    for (int m = 0; m < NB_MODES; m++)
        for (const EntreeClassement& e : tables[static_cast<size_t>(m)])
            sortie << mode::Identifiant(static_cast<Mode>(m)) << ';' << e.score << ';' << e.lignes << ';' << e.tempsMs
                   << ';' << e.date << '\n';
    return sortie.str();
}

Classements Classements::Analyser(const std::string& contenu) {
    Classements resultat;
    std::istringstream flux(contenu);
    std::string ligne;

    while (std::getline(flux, ligne)) {
        if (!ligne.empty() && ligne.back() == '\r') ligne.pop_back();
        if (ligne.empty() || ligne[0] == '#') continue;

        std::vector<std::string> champs;
        std::istringstream morceaux(ligne);
        std::string champ;
        while (champs.size() < 6 && std::getline(morceaux, champ, ';')) champs.push_back(champ);
        if (champs.size() < 4 || champs.size() > 5) continue;

        const auto m = mode::DepuisIdentifiant(champs[0]);
        EntreeClassement e;
        if (!m || !Nombre(champs[1], e.score) || !Nombre(champs[2], e.lignes) || !Nombre(champs[3], e.tempsMs)) continue;
        if (e.score < 0 || e.lignes < 0 || e.tempsMs < 0) continue;
        if (champs.size() == 5) {
            if (!DateValide(champs[4])) continue;
            e.date = champs[4];
        }
        resultat.Ajouter(*m, e);
    }
    return resultat;
}

namespace classements {

std::filesystem::path CheminFichier() {
    return fichiers::DossierDonnees() / "classements.txt";
}

Classements Charger(const std::filesystem::path& chemin) {
    const auto contenu = fichiers::LireTout(chemin);
    return contenu ? Classements::Analyser(*contenu) : Classements{};
}

bool Sauvegarder(const std::filesystem::path& chemin, const Classements& table) {
    return fichiers::EcrireAtomique(chemin, table.Serialiser());
}

std::string DateActuelle() {
    const std::time_t maintenant = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    if (localtime_s(&local, &maintenant) != 0) return {};
#else
    if (!localtime_r(&maintenant, &local)) return {};
#endif
    char tampon[20];
    return std::strftime(tampon, sizeof tampon, "%Y-%m-%d %H:%M", &local) ? std::string(tampon) : std::string{};
}

} // namespace classements
