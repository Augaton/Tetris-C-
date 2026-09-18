#include "Mode.h"

#include <array>

namespace {

constexpr std::array<const char*, NB_MODES> IDENTIFIANTS = {"marathon", "sprint", "ultra", "zen"};

} // namespace

namespace mode {

const char* Identifiant(Mode m) {
    return IDENTIFIANTS[static_cast<size_t>(m)];
}

std::optional<Mode> DepuisIdentifiant(const std::string& identifiant) {
    for (size_t i = 0; i < IDENTIFIANTS.size(); i++)
        if (identifiant == IDENTIFIANTS[i]) return static_cast<Mode>(i);
    return std::nullopt;
}

} // namespace mode
