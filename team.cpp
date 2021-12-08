#include "team.hpp"

Team::Team(TeamID id, const std::string& displayName, std::optional<std::shared_ptr<Networking::Session>> session)
: id(id), displayName(displayName), resourceUnits(0), session(session) { }