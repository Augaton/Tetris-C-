#include "Reglages.h"

#include "Fichiers.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>
#include <utility>
#include <vector>

namespace {

constexpr std::array<const char*, NB_ACTIONS> IDENTIFIANTS = {
    "gauche", "droite", "descente_douce", "chute_rapide", "tourner_horaire",
    "tourner_antihoraire", "garder", "pause", "abandonner",
};

std::string Nettoyer(const std::string& texte) {
    size_t debut = 0, fin = texte.size();
    while (debut < fin && std::isspace(static_cast<unsigned char>(texte[debut]))) debut++;
    while (fin > debut && std::isspace(static_cast<unsigned char>(texte[fin - 1]))) fin--;
    return texte.substr(debut, fin - debut);
}

std::optional<int> Entier(const std::string& texte) {
    int valeur = 0;
    const char* fin = texte.data() + texte.size();
    auto [ptr, erreur] = std::from_chars(texte.data(), fin, valeur);
    if (erreur != std::errc{} || ptr != fin) return std::nullopt;
    return valeur;
}

std::optional<bool> Booleen(const std::string& texte) {
    if (texte == "oui" || texte == "1" || texte == "true") return true;
    if (texte == "non" || texte == "0" || texte == "false") return false;
    return std::nullopt;
}

} // namespace

Reglages::TableTouches Reglages::TouchesVides() {
    TableTouches vides;
    for (auto& touchesAction : vides) touchesAction.fill(AUCUNE_TOUCHE);
    return vides;
}

const std::array<int, TOUCHES_PAR_ACTION>& Reglages::Touches(Action action) const {
    return touches[static_cast<int>(action)];
}

void Reglages::AssignerTouche(Action action, int code) {
    if (code == AUCUNE_TOUCHE) return;

    for (auto& touchesAction : touches) {
        for (int& t : touchesAction) {
            if (t == code) t = AUCUNE_TOUCHE;
        }
        // Garde la principale remplie si seule la secondaire reste
        if (touchesAction[0] == AUCUNE_TOUCHE) std::swap(touchesAction[0], touchesAction[1]);
    }

    auto& cible = touches[static_cast<int>(action)];
    cible[1] = cible[0];
    cible[0] = code;
}

void Reglages::EffacerTouches(Action action) {
    touches[static_cast<int>(action)].fill(AUCUNE_TOUCHE);
}

std::optional<Action> Reglages::ActionDe(int code) const {
    if (code == AUCUNE_TOUCHE) return std::nullopt;
    for (int a = 0; a < NB_ACTIONS; a++)
        for (int t : touches[a])
            if (t == code) return static_cast<Action>(a);
    return std::nullopt;
}

void Reglages::ReinitialiserOptions() {
    const Reglages defaut;
    dasMs = defaut.dasMs;
    arrMs = defaut.arrMs;
    descenteDouceMs = defaut.descenteDouceMs;
    verrouillageMs = defaut.verrouillageMs;
    fantome = defaut.fantome;
    effets = defaut.effets;
    secousses = defaut.secousses;
    pleinEcran = defaut.pleinEcran;
}

namespace reglages {

const char* Identifiant(Action action) {
    return IDENTIFIANTS[static_cast<int>(action)];
}

std::filesystem::path CheminFichier() {
    return fichiers::DossierDonnees() / "reglages.cfg";
}

Reglages Analyser(const std::string& contenu, const Reglages& defauts, const NomVersCode& codeTouche) {
    Reglages r = defauts;
    std::istringstream flux(contenu);
    std::string ligne;

    while (std::getline(flux, ligne)) {
        ligne = Nettoyer(ligne);
        if (ligne.empty() || ligne[0] == '#') continue;

        const size_t egal = ligne.find('=');
        if (egal == std::string::npos) continue;
        const std::string cle = Nettoyer(ligne.substr(0, egal));
        const std::string valeur = Nettoyer(ligne.substr(egal + 1));

        auto nombre = [&](int& champ, const Bornes& bornes) {
            if (auto v = Entier(valeur)) champ = bornes.Limiter(*v);
        };
        auto booleen = [&](bool& champ) {
            if (auto v = Booleen(valeur)) champ = *v;
        };

        if (cle == "das_ms") nombre(r.dasMs, Reglages::BORNES_DAS);
        else if (cle == "arr_ms") nombre(r.arrMs, Reglages::BORNES_ARR);
        else if (cle == "descente_douce_ms") nombre(r.descenteDouceMs, Reglages::BORNES_DESCENTE);
        else if (cle == "verrouillage_ms") nombre(r.verrouillageMs, Reglages::BORNES_VERROUILLAGE);
        else if (cle == "fantome") booleen(r.fantome);
        else if (cle == "effets") booleen(r.effets);
        else if (cle == "secousses") booleen(r.secousses);
        else if (cle == "plein_ecran") booleen(r.pleinEcran);
        else if (cle.rfind("touche.", 0) == 0) {
            const std::string id = cle.substr(7);
            for (int a = 0; a < NB_ACTIONS; a++) {
                if (id != IDENTIFIANTS[a]) continue;

                std::vector<int> codes;
                std::istringstream noms(valeur);
                std::string nom;
                while (std::getline(noms, nom, ',')) {
                    if (auto code = codeTouche(Nettoyer(nom))) codes.push_back(*code);
                }
                // Valeur vide = aucune touche voulue ; noms tous invalides = on garde le défaut
                if (!valeur.empty() && codes.empty()) break;

                const Action action = static_cast<Action>(a);
                r.EffacerTouches(action);
                // En ordre inverse : la première touche listée devient la principale
                for (int i = std::min<int>(static_cast<int>(codes.size()), TOUCHES_PAR_ACTION) - 1; i >= 0; i--)
                    r.AssignerTouche(action, codes[i]);
                break;
            }
        }
    }
    return r;
}

std::string Serialiser(const Reglages& r, const CodeVersNom& nomTouche) {
    std::ostringstream sortie;
    sortie << "# Réglages de Tetris (modifiables aussi depuis le jeu)\n"
           << "# Durées en millisecondes, 0 = instantané\n"
           << "das_ms = " << r.dasMs << '\n'
           << "arr_ms = " << r.arrMs << '\n'
           << "descente_douce_ms = " << r.descenteDouceMs << '\n'
           << "verrouillage_ms = " << r.verrouillageMs << '\n'
           << "fantome = " << (r.fantome ? "oui" : "non") << '\n'
           << "effets = " << (r.effets ? "oui" : "non") << '\n'
           << "secousses = " << (r.secousses ? "oui" : "non") << '\n'
           << "plein_ecran = " << (r.pleinEcran ? "oui" : "non") << '\n'
           << "\n# Touches : jusqu'à " << TOUCHES_PAR_ACTION << " par action, séparées par des virgules\n";

    for (int a = 0; a < NB_ACTIONS; a++) {
        sortie << "touche." << IDENTIFIANTS[a] << " =";
        bool premiere = true;
        for (int code : r.touches[a]) {
            if (code == AUCUNE_TOUCHE) continue;
            sortie << (premiere ? " " : ", ") << nomTouche(code);
            premiere = false;
        }
        sortie << '\n';
    }
    return sortie.str();
}

Reglages Charger(const std::filesystem::path& chemin, const Reglages& defauts, const NomVersCode& codeTouche) {
    const auto contenu = fichiers::LireTout(chemin);
    return contenu ? Analyser(*contenu, defauts, codeTouche) : defauts;
}

bool Sauvegarder(const std::filesystem::path& chemin, const Reglages& r, const CodeVersNom& nomTouche) {
    return fichiers::EcrireAtomique(chemin, Serialiser(r, nomTouche));
}

} // namespace reglages
