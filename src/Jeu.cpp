#include "Jeu.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

bool Pleine(const std::array<int, cst::LARGEUR>& ligne) {
    return std::ranges::all_of(ligne, [](int v) { return v != 0; });
}

bool Occupee(const std::array<int, cst::LARGEUR>& ligne) {
    return std::ranges::any_of(ligne, [](int v) { return v != 0; });
}

} // namespace

Jeu::Jeu(unsigned graine, const Grille& depart, ParametresPartie parametresPartie)
    : grille(depart), parametres(parametresPartie), graine(graine), sac(graine) {
    evenements.reserve(32);
    parametres.niveauDepart = std::clamp(parametres.niveauDepart, 0, mode::NIVEAU_DEPART_MAX);
    if (parametres.mode == Mode::Marathon) niveau = parametres.niveauDepart;
    TypePiece premiere = sac.Tirer();
    suivante = sac.Tirer();
    Apparaitre(premiere);
}

void Jeu::ActiverEnregistrement() {
    enregistrer = true;
    journal = {graine, parametres, {}, true};
    journal.commandes.reserve(1 << 14);
    Noter(Commande::Type::DelaiVerrouillage, 0, delaiVerrouillage);
}

void Jeu::Noter(Commande::Type type, int argument, float valeur) {
    if (!enregistrer) return;
    if (journal.commandes.size() >= JOURNAL_MAX) {
        journal.complet = false;
        return;
    }
    journal.commandes.push_back({type, static_cast<std::int8_t>(argument), valeur});
}

void Jeu::Rejouer(const Commande& c) {
    switch (c.type) {
        case Commande::Type::Deplacer:          Deplacer(c.argument); break;
        case Commande::Type::DescenteDouce:     DescenteDouce(); break;
        case Commande::Type::ChuteRapide:       ChuteRapide(); break;
        case Commande::Type::Tourner:           Tourner(c.argument != 0); break;
        case Commande::Type::Garder:            Garder(); break;
        case Commande::Type::Temps:             MettreAJour(c.valeur); break;
        case Commande::Type::DelaiVerrouillage: DefinirDelaiVerrouillage(c.valeur); break;
    }
}

void Jeu::DefinirDelaiVerrouillage(float secondes) {
    delaiVerrouillage = secondes < 0.f ? 0.f : secondes;
    Noter(Commande::Type::DelaiVerrouillage, 0, delaiVerrouillage);
}

float Jeu::TempsRestant() const {
    return parametres.mode == Mode::Ultra ? std::max(0.f, mode::ULTRA_DUREE_S - temps) : 0.f;
}

void Jeu::Signaler(const EvenementJeu& evenement) {
    if (evenements.size() >= 32) evenements.erase(evenements.begin()); // personne ne les lit (tests)
    evenements.push_back(evenement);
}

void Jeu::AjouterScore(long long points) {
    // Saturation plutôt que dépassement (comportement indéfini) sur une partie interminable
    const long long maximum = std::numeric_limits<long long>::max();
    score = (points > 0 && score > maximum - points) ? maximum : score + points;
}

Cases Jeu::Placer(const EtatPiece& piece) {
    Cases cases = piece::Forme(piece.type, piece.rotation);
    for (Case& c : cases) {
        c.x += piece.x;
        c.y += piece.y;
    }
    return cases;
}

bool Jeu::Libre(const Cases& cases) const {
    for (const Case& c : cases) {
        if (c.x < 0 || c.x >= cst::LARGEUR || c.y < 0 || c.y >= cst::HAUTEUR) return false;
        if (grille[c.y][c.x] != 0) return false;
    }
    return true;
}

bool Jeu::AuSol() const {
    EtatPiece dessous = active;
    dessous.y++;
    return !Libre(Placer(dessous));
}

void Jeu::ApresMouvement() {
    if (active.y > ligneLaPlusBasse) {
        ligneLaPlusBasse = active.y;
        reinitialisations = 0;
    }
    if (chronoVerrouillage > 0.f && reinitialisations < REINITIALISATIONS_MAX) {
        chronoVerrouillage = 0.f;
        reinitialisations++;
    }
}

Cases Jeu::CasesPiece() const {
    return Placer(active);
}

Cases Jeu::CasesFantome() const {
    EtatPiece fantome = active;
    while (true) {
        fantome.y++;
        if (!Libre(Placer(fantome))) break;
    }
    fantome.y--;
    return Placer(fantome);
}

void Jeu::Apparaitre(TypePiece type) {
    active = {type, 0, piece::ColonneDepart(type), 0};
    numeroPiece++;
    chronoGravite = 0.f;
    chronoVerrouillage = 0.f;
    reinitialisations = 0;
    ligneLaPlusBasse = 0;
    if (!Libre(CasesPiece())) perdu = true;
}

bool Jeu::Deplacer(int dx) {
    Noter(Commande::Type::Deplacer, dx);
    if (Fini()) return false;
    EtatPiece essai = active;
    essai.x += dx;
    if (!Libre(Placer(essai))) return false;
    active = essai;
    ApresMouvement();
    return true;
}

bool Jeu::DescenteDouce() {
    Noter(Commande::Type::DescenteDouce);
    if (Fini()) return false;
    EtatPiece essai = active;
    essai.y++;
    if (!Libre(Placer(essai))) return false;
    active = essai;
    ApresMouvement();
    AjouterScore(niveau + 1);
    chronoGravite = 0.f;
    return true;
}

void Jeu::ChuteRapide() {
    Noter(Commande::Type::ChuteRapide);
    if (Fini()) return;
    int distance = 0;
    EtatPiece essai = active;
    while (true) {
        essai.y++;
        if (!Libre(Placer(essai))) break;
        distance++;
    }
    active.y += distance;
    AjouterScore(2LL * std::max(1, niveau) * distance);

    Signaler({.type = EvenementJeu::Type::ChuteRapide,
              .cases = CasesPiece(),
              .couleur = piece::Couleur(active.type),
              .distance = distance});

    Verrouiller();
}

bool Jeu::Tourner(bool horaire) {
    Noter(Commande::Type::Tourner, horaire ? 1 : 0);
    if (Fini() || active.type == TypePiece::O) return false;

    EtatPiece tourne = active;
    tourne.rotation = (active.rotation + (horaire ? 1 : 3)) % 4;

    for (const Case& decalage : piece::Decalages(active.type, active.rotation, horaire)) {
        EtatPiece essai = tourne;
        essai.x += decalage.x;
        essai.y += decalage.y;
        if (Libre(Placer(essai))) {
            active = essai;
            ApresMouvement();
            return true;
        }
    }
    return false;
}

void Jeu::Garder() {
    Noter(Commande::Type::Garder);
    if (Fini() || gardeUtilisee) return;
    gardeUtilisee = true;

    TypePiece courante = active.type;
    if (garde) {
        Apparaitre(*garde);
    } else {
        Apparaitre(suivante);
        suivante = sac.Tirer();
    }
    garde = courante;
}

void Jeu::MettreAJour(float dt) {
    Noter(Commande::Type::Temps, 0, dt);
    if (Fini()) return;

    temps += dt;
    if (parametres.mode == Mode::Ultra && temps >= mode::ULTRA_DUREE_S) {
        temps = mode::ULTRA_DUREE_S;
        objectifAtteint = true;
        return;
    }

    if (combo > 0) {
        comboRestant -= dt;
        if (comboRestant <= 0.f) {
            combo = 0;
            comboRestant = 0.f;
        }
    }

    if (AuSol()) {
        // Au sol : la gravité ne fait rien, seul le délai de verrouillage compte
        chronoGravite = 0.f;
        chronoVerrouillage += dt;
        if (chronoVerrouillage >= delaiVerrouillage) Verrouiller();
        return;
    }

    chronoVerrouillage = 0.f;
    if (parametres.mode == Mode::Zen) return; // pas de gravité : la pièce ne descend que si on le demande

    // On garde le reste du chrono : cadence de chute régulière, indépendante de la fréquence d'images
    const float intervalle = IntervalleGravite();
    chronoGravite += dt;
    if (chronoGravite < intervalle) return;
    chronoGravite -= intervalle;
    if (chronoGravite >= intervalle) chronoGravite = 0.f; // jamais plus d'une case par image

    active.y++;
    ApresMouvement();
}

float Jeu::ProgressionVerrouillage() const {
    if (Fini() || delaiVerrouillage <= 0.f || !AuSol()) return 0.f;
    return std::min(1.f, chronoVerrouillage / delaiVerrouillage);
}

float Jeu::IntervalleGravite() const {
    float intervalle = cst::GRAVITE_INITIALE_S * std::pow(cst::GRAVITE_FACTEUR, static_cast<float>(niveau));
    return std::max(intervalle, cst::GRAVITE_MIN_S);
}

void Jeu::Verrouiller() {
    const int couleur = piece::Couleur(active.type);
    for (const Case& c : CasesPiece()) grille[c.y][c.x] = couleur;
    stats.pieces++;

    Signaler({.type = EvenementJeu::Type::Verrouillage, .cases = CasesPiece(), .couleur = couleur});

    // Lignes pleines relevées avant l'effacement, pour les effets
    EvenementJeu lignesPleines{.type = EvenementJeu::Type::Lignes};
    for (int y = 0; y < cst::HAUTEUR && lignesPleines.nbLignes < 4; y++) {
        if (Pleine(grille[y])) {
            lignesPleines.lignes[lignesPleines.nbLignes] = y;
            lignesPleines.contenu[lignesPleines.nbLignes] = grille[y];
            lignesPleines.nbLignes++;
        }
    }

    const int niveauAvant = niveau;
    const long long scoreAvant = score;
    int effacees = EffacerLignes();
    if (effacees > 0) {
        AjouterLignes(effacees);
        lignesPleines.points = score - scoreAvant;
        Signaler(lignesPleines);
    }
    if (niveau > niveauAvant) Signaler({.type = EvenementJeu::Type::Niveau, .niveau = niveau});

    if (parametres.mode == Mode::Sprint && lignes >= parametres.sprintLignes) {
        objectifAtteint = true;
        return;
    }

    // Perdu s'il reste un bloc au-dessus de la ligne limite ; en Zen, la pile est vidée à la place
    if (std::ranges::any_of(std::span(grille).first<cst::LIGNES_ZONE_LIMITE>(), Occupee)) {
        if (parametres.mode != Mode::Zen) {
            perdu = true;
            return;
        }
        for (auto& ligne : grille) ligne.fill(0);
        Signaler({.type = EvenementJeu::Type::Nettoyage});
    }

    gardeUtilisee = false;
    Apparaitre(suivante);
    suivante = sac.Tirer();
}

int Jeu::EffacerLignes() {
    int effacees = 0;
    int ecriture = cst::HAUTEUR - 1;

    for (int y = cst::HAUTEUR - 1; y >= 0; y--) {
        if (Pleine(grille[y])) {
            effacees++;
            continue;
        }
        if (ecriture != y) grille[ecriture] = grille[y];
        ecriture--;
    }
    for (; ecriture >= 0; ecriture--) grille[ecriture].fill(0);

    return effacees;
}

void Jeu::AjouterLignes(int nombre) {
    lignes += nombre;
    // Seul le Marathon monte de niveau ; les autres modes gardent une gravité constante
    if (parametres.mode == Mode::Marathon)
        niveau = std::min(cst::NIVEAU_MAX, parametres.niveauDepart + lignes / cst::LIGNES_PAR_NIVEAU);

    combo++;
    comboRestant = cst::COMBO_DUREE_S;
    stats.lignesParType[static_cast<size_t>(std::clamp(nombre, 1, 4) - 1)]++;
    stats.comboMax = std::max(stats.comboMax, combo);

    AjouterScore(nombre * 100LL + 100LL * (nombre - 1) + combo * 50LL);
}
