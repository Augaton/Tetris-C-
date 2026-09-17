#include "Ressources.h"

#include <iostream>
#include <system_error>
#include <vector>

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#elif defined(__APPLE__)
#  include <mach-o/dyld.h>
#endif

namespace fs = std::filesystem;

namespace ressources {

fs::path DossierExecutable() {
#ifdef _WIN32
    std::wstring tampon(MAX_PATH, L'\0');
    while (true) {
        DWORD longueur = GetModuleFileNameW(nullptr, tampon.data(), static_cast<DWORD>(tampon.size()));
        if (longueur == 0) return {};
        if (longueur < tampon.size()) {
            tampon.resize(longueur);
            return fs::path(tampon).parent_path();
        }
        tampon.resize(tampon.size() * 2); // chemin tronqué : on agrandit
    }
#elif defined(__APPLE__)
    uint32_t taille = 0;
    _NSGetExecutablePath(nullptr, &taille);
    std::string tampon(taille, '\0');
    if (_NSGetExecutablePath(tampon.data(), &taille) != 0) return {};
    std::error_code erreur;
    fs::path chemin = fs::weakly_canonical(tampon.c_str(), erreur);
    return erreur ? fs::path{} : chemin.parent_path();
#else
    std::error_code erreur;
    fs::path chemin = fs::read_symlink("/proc/self/exe", erreur);
    return erreur ? fs::path{} : chemin.parent_path();
#endif
}

std::optional<fs::path> Trouver(const std::string& nom) {
    std::vector<fs::path> dossiers;

    // Jamais le dossier courant : lancé depuis un dossier piégé, le jeu y chargerait
    // des images ou une police malveillantes
    const fs::path exe = DossierExecutable();
    if (!exe.empty()) {
        dossiers.push_back(exe / "asset");
        dossiers.push_back(exe.parent_path() / "asset");
    }
    std::error_code erreur;

    for (const fs::path& dossier : dossiers) {
        fs::path candidat = dossier / nom;
        if (fs::is_regular_file(candidat, erreur)) return candidat;
    }

    if (exe.empty()) std::cerr << "Impossible de déterminer le dossier de l'exécutable\n";
    std::cerr << "Ressource introuvable : " << nom << "\nDossiers essayés :\n";
    for (const fs::path& dossier : dossiers) std::cerr << "  " << dossier.string() << '\n';
    return std::nullopt;
}

} // namespace ressources
