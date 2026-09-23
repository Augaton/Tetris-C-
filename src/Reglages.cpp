#include "Reglages.h"

#include "Fichiers.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <ranges>
#include <sstream>
#include <utility>
#include <vector>

namespace {

constexpr std::array<std::string_view, NB_ACTIONS> IDENTIFIANTS = {
    "gauche", "droite", "descente_douce", "chute_rapide", "tourner_horaire",
    "tourner_antihoraire", "garder", "pause", "abandonner",
};

// Sans les espaces du début et de la fin (vue sur le texte d'origine, sans copie)
std::string_view Nettoyer(std::string_view texte) {
    const auto espace = [](char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; };
    while (!texte.empty() && espace(texte.front())) texte.remove_prefix(1);
    while (!texte.empty() && espace(texte.back())) texte.remove_suffix(1);
    return texte;
}

std::optional<int> Entier(std::string_view texte) {
    int valeur = 0;
    const char* fin = texte.data() + texte.size();
    const auto [ptr, erreur] = std::from_chars(texte.data(), fin, valeur);
    if (erreur != std::errc{} || ptr != fin) return std::nullopt;
    return valeur;
}

void Assigner(Reglages::TableTouches& table, Action action, int code) {
    if (code == AUCUNE_TOUCHE) return;

    for (auto& touchesAction : table) {
        for (int& t : touchesAction) {
            if (t == code) t = AUCUNE_TOUCHE;
        }
        // Garde la principale remplie si seule la secondaire reste
        if (touchesAction[0] == AUCUNE_TOUCHE) std::swap(touchesAction[0], touchesAction[1]);
    }

    auto& cible = table[static_cast<size_t>(action)];
    cible[1] = cible[0];
    cible[0] = code;
}

std::optional<bool> Booleen(std::string_view texte) {
    if (texte == "oui" || texte == "1" || texte == "true") return true;
    if (texte == "non" || texte == "0" || texte == "false") return false;
    return std::nullopt;
}

// « A, Left » : noms séparés par des virgules, le premier devient la touche principale
void LireTouches(Reglages::TableTouches& table, Action action, std::string_view valeur,
                 const reglages::NomVersCode& codeTouche) {
    std::vector<int> codes;
    for (const auto nom : std::views::split(valeur, ','))
        if (auto code = codeTouche(Nettoyer(std::string_view(nom.begin(), nom.end())))) codes.push_back(*code);
    // Valeur vide = aucune touche voulue ; noms tous invalides = on garde le défaut
    if (!valeur.empty() && codes.empty()) return;

    table[static_cast<size_t>(action)].fill(AUCUNE_TOUCHE);
    // En ordre inverse : la première listée devient la principale
    for (int i = std::min<int>(static_cast<int>(codes.size()), TOUCHES_PAR_ACTION) - 1; i >= 0; i--)
        Assigner(table, action, codes[static_cast<size_t>(i)]);
}

} // namespace

Reglages::TableTouches Reglages::TouchesVides() {
    TableTouches vides;
    for (auto& touchesAction : vides) touchesAction.fill(AUCUNE_TOUCHE);
    return vides;
}

const std::array<int, TOUCHES_PAR_ACTION>& Reglages::Touches(Action action) const {
    return touches[static_cast<size_t>(action)];
}

const std::array<int, TOUCHES_PAR_ACTION>& Reglages::Boutons(Action action) const {
    return manette[static_cast<size_t>(action)];
}

void Reglages::AssignerTouche(Action action, int code) {
    Assigner(touches, action, code);
}

void Reglages::AssignerBouton(Action action, int code) {
    Assigner(manette, action, code);
}

void Reglages::EffacerTouches(Action action) {
    touches[static_cast<size_t>(action)].fill(AUCUNE_TOUCHE);
}

void Reglages::EffacerBoutons(Action action) {
    manette[static_cast<size_t>(action)].fill(AUCUNE_TOUCHE);
}

std::optional<Action> Reglages::ActionDe(int code) const {
    if (code == AUCUNE_TOUCHE) return std::nullopt;
    for (const TableTouches* table : {&touches, &manette})
        for (int a = 0; a < NB_ACTIONS; a++)
            for (int t : (*table)[static_cast<size_t>(a)])
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
    mouvementsFluides = defaut.mouvementsFluides;
    synchroVerticale = defaut.synchroVerticale;
    limiteImages = defaut.limiteImages;
    effets = defaut.effets;
    secousses = defaut.secousses;
    pleinEcran = defaut.pleinEcran;
    rotationAnticipee = defaut.rotationAnticipee;
    daltonien = defaut.daltonien;
    motifs = defaut.motifs;
    tailleTexte = defaut.tailleTexte;
}

namespace reglages {

std::string_view Identifiant(Action action) {
    return IDENTIFIANTS[static_cast<size_t>(action)];
}

std::filesystem::path CheminFichier() {
    return fichiers::DossierDonnees() / "reglages.cfg";
}

Reglages Analyser(const std::string& contenu, const Reglages& defauts, const NomVersCode& codeTouche) {
    Reglages r = defauts;
    std::istringstream flux(contenu);
    std::string brute;

    while (std::getline(flux, brute)) {
        const std::string_view ligne = Nettoyer(brute);
        if (ligne.empty() || ligne.front() == '#') continue;

        const size_t egal = ligne.find('=');
        if (egal == std::string_view::npos) continue;
        const std::string_view cle = Nettoyer(ligne.substr(0, egal));
        const std::string_view valeur = Nettoyer(ligne.substr(egal + 1));

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
        else if (cle == "mouvements_fluides") booleen(r.mouvementsFluides);
        else if (cle == "synchro_verticale") booleen(r.synchroVerticale);
        else if (cle == "limite_images") {
            nombre(r.limiteImages, Reglages::BORNES_LIMITE_IMAGES);
            if (r.limiteImages > 0 && r.limiteImages < 30) r.limiteImages = 30; // pas de jeu injouable
        }
        else if (cle == "effets") booleen(r.effets);
        else if (cle == "secousses") booleen(r.secousses);
        else if (cle == "plein_ecran") booleen(r.pleinEcran);
        else if (cle == "rotation_anticipee") booleen(r.rotationAnticipee);
        else if (cle == "daltonien") booleen(r.daltonien);
        else if (cle == "motifs") booleen(r.motifs);
        else if (cle == "taille_texte") {
            nombre(r.tailleTexte, Reglages::BORNES_TAILLE_TEXTE);
            // Aligné sur les paliers proposés dans le jeu
            const Bornes& b = Reglages::BORNES_TAILLE_TEXTE;
            r.tailleTexte = b.min + (r.tailleTexte - b.min) / b.pas * b.pas;
        }
        else if (cle == "langue") {
            if (valeur == "fr") r.langue = Langue::Francais;
            else if (valeur == "en") r.langue = Langue::Anglais;
        }
        else if (cle == "mode") {
            if (auto m = mode::DepuisIdentifiant(valeur)) r.mode = *m;
        }
        else if (cle == "niveau_depart") nombre(r.niveauDepart, Bornes{0, mode::NIVEAU_DEPART_MAX, 1});
        else if (cle.starts_with("touche.") || cle.starts_with("manette.")) {
            const bool estManette = cle.starts_with("manette.");
            const auto action = std::ranges::find(IDENTIFIANTS, cle.substr(cle.find('.') + 1));
            if (action == IDENTIFIANTS.end()) continue;
            LireTouches(estManette ? r.manette : r.touches, static_cast<Action>(action - IDENTIFIANTS.begin()), valeur,
                        codeTouche);
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
           << "mouvements_fluides = " << (r.mouvementsFluides ? "oui" : "non") << '\n'
           << "synchro_verticale = " << (r.synchroVerticale ? "oui" : "non") << '\n'
           << "limite_images = " << r.limiteImages << "  # 0 = automatique\n"
           << "effets = " << (r.effets ? "oui" : "non") << '\n'
           << "secousses = " << (r.secousses ? "oui" : "non") << '\n'
           << "plein_ecran = " << (r.pleinEcran ? "oui" : "non") << '\n'
           << "rotation_anticipee = " << (r.rotationAnticipee ? "oui" : "non") << '\n'
           << "daltonien = " << (r.daltonien ? "oui" : "non") << '\n'
           << "motifs = " << (r.motifs ? "oui" : "non") << '\n'
           << "taille_texte = " << r.tailleTexte << '\n'
           << "langue = " << (r.langue == Langue::Anglais ? "en" : "fr") << '\n'
           << "mode = " << mode::Identifiant(r.mode) << '\n'
           << "niveau_depart = " << r.niveauDepart << '\n'
           << "\n# Touches et boutons : jusqu'à " << TOUCHES_PAR_ACTION << " par action, séparés par des virgules\n";

    auto ecrireTable = [&](std::string_view prefixe, const Reglages::TableTouches& table) {
        for (int a = 0; a < NB_ACTIONS; a++) {
            sortie << prefixe << IDENTIFIANTS[static_cast<size_t>(a)] << " =";
            bool premiere = true;
            for (int code : table[static_cast<size_t>(a)]) {
                if (code == AUCUNE_TOUCHE) continue;
                sortie << (premiere ? " " : ", ") << nomTouche(code);
                premiere = false;
            }
            sortie << '\n';
        }
    };
    ecrireTable("touche.", r.touches);
    ecrireTable("manette.", r.manette);
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
