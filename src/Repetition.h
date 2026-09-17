#pragma once

// Répétition d'une touche tenue (DAS/ARR) : un déclenchement à l'appui,
// puis après `delaiInitial`, un déclenchement toutes les `intervalle` secondes.
// Un intervalle nul déclenche `maximum` fois d'un coup (déplacement instantané).
class RepetitionTouche {
public:
    RepetitionTouche(float delaiInitial, float intervalle, int maximum);

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
