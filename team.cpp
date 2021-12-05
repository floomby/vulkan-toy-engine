#include "team.hpp"

Team::Team(TeamID id, const std::string& displayName)
: id(id), displayName(displayName), resourceUnits(0) { }