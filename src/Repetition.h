#pragma once

// Répétition d'une touche tenue (DAS/ARR) : un déclenchement à l'appui,
// puis après `delai`, un déclenchement toutes les `periode` secondes.
// Une période nulle déclenche `plafond` fois d'un coup (déplacement instantané).
class RepetitionTouche {
public:
    RepetitionTouche(float delai, float periode, int plafond);

    void Appuyer(); // (re)démarre la répétition
    void Relacher();
    bool Enfoncee() const { return enfoncee; }

    // Nombre de déclenchements pendant `dt`
    int MettreAJour(float dt);

private:
    float delaiInitial;
    float intervalle;
    int maximum;
    float attente = 0.f;
    bool enfoncee = false;
    bool premier = false;
};
