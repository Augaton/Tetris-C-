#include "Repetition.h"

RepetitionTouche::RepetitionTouche(float delai, float periode, int plafond)
    : delaiInitial(delai), intervalle(periode), maximum(plafond) {}

void RepetitionTouche::Appuyer() {
    enfoncee = true;
    premier = true;
    attente = delaiInitial;
}

void RepetitionTouche::Relacher() {
    enfoncee = false;
    premier = false;
}

int RepetitionTouche::MettreAJour(float dt) {
    if (!enfoncee) return 0;
    if (premier) {
        premier = false;
        return 1;
    }

    attente -= dt;
    if (attente > 0.f) return 0;
    if (intervalle <= 0.f) return maximum;

    int declenchements = 0;
    while (attente <= 0.f && declenchements < maximum) {
        declenchements++;
        attente += intervalle;
    }
    if (attente <= 0.f) attente = intervalle; // gros ralentissement : on ne rattrape pas tout
    return declenchements;
}
