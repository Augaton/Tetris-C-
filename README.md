# Tetris

[![Compilation](https://github.com/Augaton/Tetris-C-/actions/workflows/compilation.yml/badge.svg)](https://github.com/Augaton/Tetris-C-/actions/workflows/compilation.yml)

Un Tetris en C++20 avec [SFML 3.1](https://www.sfml-dev.org/), pour Linux, Windows et macOS.

## Fonctionnalités

- **Quatre modes** : Marathon (niveau de départ au choix), Sprint 40 lignes, Ultra 2 minutes, Zen (sans gravité ni défaite).
- **Règles modernes** : rotations SRS avec décalages contre les murs, pièce gardée, pièce fantôme, tirage par sacs de 7, délai de verrouillage.
- **Réglages fins** : délai et vitesse de répétition (DAS/ARR), descente rapide, rotation et garde anticipées (IRS/IHS), limite d'images par seconde.
- **Classements** : top 10 local par mode, et revisionnage de la dernière partie.
- **Manette** prise en charge, touches et boutons réattribuables.
- **Accessibilité** : palette pour daltoniens, motifs sur les pièces, taille du texte des menus.
- **Français et anglais**, selon la langue du système.

## Commandes par défaut

| Action                 | Clavier            | Manette          |
| ---------------------- | ------------------ | ---------------- |
| Déplacer               | ← →                | Croix ou stick   |
| Descente rapide        | ↓                  | Croix ou stick ↓ |
| Chute directe          | Espace             | Croix ↑          |
| Tourner (horaire)      | Entrée ou ↑        | A ou Y           |
| Tourner (anti-horaire) | Ctrl droit         | B ou X           |
| Garder la pièce        | Maj droit ou C     | LB ou RB         |
| Pause                  | P ou Échap         | Start            |
| Abandonner             | A                  | Back             |
| Plein écran            | F11                |                  |

Elles se changent dans le menu **Commandes**.

## Télécharger

Le jeu prêt à jouer, pour Linux, Windows et macOS, est publié avec chaque version dans les
[Releases](https://github.com/Augaton/Tetris-C-/releases). Chaque compilation de l'onglet Actions le propose aussi
dans ses artefacts (connexion à GitHub nécessaire).

- **Windows** : décompresser `Tetris-Windows.zip` et lancer `Tetris.exe`.
- **Linux** : `tar xzf Tetris-Linux.tar.gz && ./Tetris/Tetris`. Compilé sous Ubuntu 24.04 : il faut une distribution
  de 2024 ou plus récente, avec FreeType et HarfBuzz (présents sur tout bureau).
- **macOS** (Mac à puce Apple, macOS 13.3 ou plus récent) : `tar xzf Tetris-macOS.tar.gz`, puis
  `xattr -dr com.apple.quarantine Tetris` car le jeu n'est pas signé, et `./Tetris/Tetris`.

Pour publier une version : `git tag v1.0 && git push origin v1.0`.

## Compiler

Il faut :

- CMake 3.28 ou plus récent ;
- un compilateur C++20 : GCC 13+, Clang 17+ (Xcode 16 sous macOS) ou Visual Studio 2022 ;
- git et une connexion internet à la première configuration : si SFML 3.1 n'est pas installée, CMake la télécharge et la compile avec le jeu ;
- sous Linux, les bibliothèques de développement suivantes.

```sh
# Fedora
sudo dnf install libX11-devel libXrandr-devel libXcursor-devel libXi-devel systemd-devel \
    mesa-libGL-devel freetype-devel harfbuzz-devel
# Debian, Ubuntu
sudo apt install libx11-dev libxrandr-dev libxcursor-dev libxi-dev libudev-dev \
    libgl1-mesa-dev libfreetype-dev libharfbuzz-dev
```

Puis :

```sh
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build     # tests de la logique du jeu
./build/Tetris
```

Avec Visual Studio, compiler avec `cmake --build build --config Release` ; le jeu est alors dans `build/Release/`.

Options CMake (`-DOPTION=ON`) :

| Option                 | Par défaut | Effet                                                              |
| ---------------------- | ---------- | ------------------------------------------------------------------ |
| `TETRIS_TESTS`         | `ON`       | Compile les tests de la logique                                    |
| `TETRIS_DURCISSEMENT`  | `ON`       | Protection de pile, `_FORTIFY_SOURCE`, vérifications de la STL     |
| `TETRIS_NATIF`         | `OFF`      | Optimise pour le processeur de la machine (binaire non portable)   |
| `TETRIS_SANITIZERS`    | `OFF`      | AddressSanitizer et UBSan (paquets `libasan` et `libubsan` requis) |
| `TETRIS_SFML_STATIQUE` | `OFF`      | Avec une SFML déjà installée, utiliser ses bibliothèques statiques |

## Réglages et classements

Ils sont enregistrés dans `reglages.cfg` (modifiable à la main) et `classements.txt`, dans le dossier :

- Linux : `$XDG_DATA_HOME/TetrisC`, sinon `~/.local/share/TetrisC` ;
- Windows : `%APPDATA%\TetrisC` ;
- macOS : `~/Library/Application Support/TetrisC`.

## Organisation du code

- `src/` : la logique du jeu (`Jeu`, `Piece`, `Sac`, `Reglages`, `Classements`...) ne dépend pas de SFML ; l'interface (`Application`, `Menu`, `Rendu`, `Effets`...) s'appuie dessus.
- `tests/` : tests de la logique, sans fenêtre.
- `asset/` : images et police, copiées à côté de l'exécutable à la compilation.

## Licences

La police Liberation Sans est distribuée sous licence SIL Open Font License 1.1 (voir `asset/LiberationSans-LICENSE.txt`).

Tetris est une marque de The Tetris Company. Ce projet personnel n'y est pas affilié.
