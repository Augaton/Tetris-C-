#include "Mode.h"

#include <algorithm>
#include <array>

namespace {

constexpr std::array<std::string_view, NB_MODES> IDENTIFIANTS = {"marathon", "sprint", "ultra", "zen"};

} // namespace

namespace mode {

std::string_view Identifiant(Mode m) {
    return IDENTIFIANTS[static_cast<std::size_t>(m)];
}

std::optional<Mode> DepuisIdentifiant(std::string_view identifiant) {
    const auto trouve = std::ranges::find(IDENTIFIANTS, identifiant);
    if (trouve == IDENTIFIANTS.end()) return std::nullopt;
    return static_cast<Mode>(trouve - IDENTIFIANTS.begin());
}

} // namespace mode
