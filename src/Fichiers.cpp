#include "Fichiers.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <system_error>

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#else
#  include <cerrno>
#  include <fcntl.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace fichiers {

fs::path DossierDonnees() {
    fs::path base;
#ifdef _WIN32
    // API Windows plutôt que _wgetenv (dépréciée) ; longueur demandée d'abord
    const DWORD longueur = GetEnvironmentVariableW(L"APPDATA", nullptr, 0);
    if (longueur > 1 && longueur < 32768) {
        std::wstring appdata(longueur, L'\0');
        const DWORD copies = GetEnvironmentVariableW(L"APPDATA", appdata.data(), longueur);
        if (copies > 0 && copies < longueur) {
            appdata.resize(copies);
            if (fs::path(appdata).is_absolute()) base = appdata;
        }
    }
#else
    // Chemins relatifs refusés : ils dépendraient du dossier depuis lequel le jeu est lancé
    const char* xdg = std::getenv("XDG_DATA_HOME");
    const char* home = std::getenv("HOME");
    if (xdg && *xdg && fs::path(xdg).is_absolute()) {
        base = xdg;
    } else if (home && *home && fs::path(home).is_absolute()) {
#  ifdef __APPLE__
        base = fs::path(home) / "Library" / "Application Support";
#  else
        base = fs::path(home) / ".local" / "share";
#  endif
    }
#endif
    return base.empty() ? base : base / "TetrisC";
}

std::optional<std::string> LireTout(const fs::path& chemin, std::size_t tailleMax) {
    std::error_code erreur;
    if (!fs::is_regular_file(chemin, erreur)) return std::nullopt; // ni dossier, ni périphérique, ni tube

    std::ifstream fichier(chemin, std::ios::binary);
    if (!fichier) return std::nullopt;

    // Lecture bornée : un fichier énorme (ou qui grossit pendant la lecture) est refusé
    std::string contenu(tailleMax + 1, '\0');
    fichier.read(contenu.data(), static_cast<std::streamsize>(contenu.size()));
    const auto lus = static_cast<std::size_t>(fichier.gcount());
    if (lus > tailleMax || fichier.bad()) return std::nullopt;
    contenu.resize(lus);
    return contenu;
}

bool EcrireAtomique(const fs::path& chemin, std::string_view contenu) {
    std::error_code erreur;
    if (chemin.has_parent_path()) {
        fs::create_directories(chemin.parent_path(), erreur);
        if (erreur) return false;
    }

    fs::path temporaire = chemin;
    temporaire += ".tmp";
    // Un ancien temporaire (ou un lien symbolique posé à sa place) est supprimé, pas suivi
    fs::remove(temporaire, erreur);

#ifdef _WIN32
    // CREATE_NEW échoue si le fichier existe : on ne réécrit jamais un fichier recréé entre-temps
    HANDLE fichier = CreateFileW(temporaire.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (fichier == INVALID_HANDLE_VALUE) return false;

    bool ok = true;
    size_t ecrits = 0;
    while (ok && ecrits < contenu.size()) {
        DWORD morceau = 0;
        const auto aEcrire = static_cast<DWORD>(std::min<size_t>(contenu.size() - ecrits, 1 << 20));
        ok = WriteFile(fichier, contenu.data() + ecrits, aEcrire, &morceau, nullptr) && morceau > 0;
        ecrits += morceau;
    }
    ok = ok && FlushFileBuffers(fichier); // données réellement sur le disque avant le renommage
    CloseHandle(fichier);

    // Remplacement atomique, attendu jusqu'à son écriture sur le disque
    ok = ok && MoveFileExW(temporaire.c_str(), chemin.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
#else
    // O_EXCL + O_NOFOLLOW : refuse un fichier ou un lien apparu entre la suppression et l'ouverture
    const int fichier = ::open(temporaire.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (fichier < 0) return false;

    bool ok = true;
    size_t ecrits = 0;
    while (ok && ecrits < contenu.size()) {
        const ssize_t morceau = ::write(fichier, contenu.data() + ecrits, contenu.size() - ecrits);
        if (morceau < 0 && errno == EINTR) continue;
        ok = morceau > 0;
        if (ok) ecrits += static_cast<size_t>(morceau);
    }
    ok = ok && ::fsync(fichier) == 0; // données réellement sur le disque avant le renommage
    ok = (::close(fichier) == 0) && ok;

    ok = ok && ::rename(temporaire.c_str(), chemin.c_str()) == 0; // remplacement atomique

    if (ok) {
        // Synchronise aussi le dossier, pour que le renommage survive à une coupure de courant
        const fs::path dossier = chemin.has_parent_path() ? chemin.parent_path() : fs::path(".");
        const int descripteur = ::open(dossier.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (descripteur >= 0) {
            ::fsync(descripteur);
            ::close(descripteur);
        }
    }
#endif

    if (!ok) fs::remove(temporaire, erreur);
    return ok;
}

} // namespace fichiers
