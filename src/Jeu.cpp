#include "Jeu.h"

#include <algorithm>
#include <cmath>

Jeu::Jeu(unsigned graine, const Grille& depart) : grille(depart), sac(graine) {
    TypePiece premiere = sac.Tirer();
    suivante = sac.Tirer();
    Apparaitre(premiere);
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
    chronoGravite = 0.f;
    chronoVerrouillage = 0.f;
    reinitialisations = 0;
    ligneLaPlusBasse = 0;
    if (!Libre(CasesPiece())) perdu = true;
}

bool Jeu::Deplacer(int dx) {
    if (perdu) return false;
    EtatPiece essai = active;
    essai.x += dx;
    if (!Libre(Placer(essai))) return false;
    active = essai;
    ApresMouvement();
    return true;
}

bool Jeu::DescenteDouce() {
    if (perdu) return false;
    EtatPiece essai = active;
    essai.y++;
    if (!Libre(Placer(essai))) return false;
    active = essai;
    ApresMouvement();
    score += niveau + 1;
    chronoGravite = 0.f;
    return true;
}

void Jeu::ChuteRapide() {
    if (perdu) return;
    int distance = 0;
    EtatPiece essai = active;
    while (true) {
        essai.y++;
        if (!Libre(Placer(essai))) break;
        distance++;
    }
    active.y += distance;
    score += 2LL * std::max(1, niveau) * distance;
    Verrouiller();
}

bool Jeu::Tourner(bool horaire) {
    if (perdu || active.type == TypePiece::O) return false;

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
    if (perdu || gardeUtilisee) return;
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
    if (perdu) return;

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
    chronoGravite += dt;
    if (chronoGravite < IntervalleGravite()) return;
    chronoGravite = 0.f;

    active.y++;
    ApresMouvement();
}

float Jeu::ProgressionVerrouillage() const {
    if (perdu || delaiVerrouillage <= 0.f || !AuSol()) return 0.f;
    return std::min(1.f, chronoVerrouillage / delaiVerrouillage);
}

float Jeu::IntervalleGravite() const {
    float intervalle = cst::GRAVITE_INITIALE_S * std::pow(cst::GRAVITE_FACTEUR, static_cast<float>(niveau));
    return std::max(intervalle, cst::GRAVITE_MIN_S);
}

void Jeu::Verrouiller() {
    const int couleur = piece::Couleur(active.type);
    for (const Case& c : CasesPiece()) grille[c.y][c.x] = couleur;

    int effacees = EffacerLignes();
    if (effacees > 0) AjouterLignes(effacees);

    // Perdu s'il reste un bloc au-dessus de la ligne limite
    for (int y = 0; y < cst::LIGNES_ZONE_LIMITE; y++) {
        for (int valeur : grille[y]) {
            if (valeur != 0) {
                perdu = true;
                return;
            }
        }
    }

    gardeUtilisee = false;
    Apparaitre(suivante);
    suivante = sac.Tirer();
}

int Jeu::EffacerLignes() {
    int effacees = 0;
    int ecriture = cst::HAUTEUR - 1;

    for (int y = cst::HAUTEUR - 1; y >= 0; y--) {
        bool pleine = std::all_of(grille[y].begin(), grille[y].end(), [](int v) { return v != 0; });
        if (pleine) {
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
    niveau = std::min(cst::NIVEAU_MAX, lignes / cst::LIGNES_PAR_NIVEAU);

    combo++;
    comboRestant = cst::COMBO_DUREE_S;

    score += nombre * 100 + 100 * (nombre - 1) + combo * 50;
}
