#pragma once

#include <cstdint>
#include <string>

#include "utilities.hpp"

class Team {
public:
    Team(TeamID id, const std::string& displayName);
    TeamID id;
    std::string displayName;
    volatile double resourceUnits;

    inline bool operator==(TeamID other) const { return id == other; }
private:

};