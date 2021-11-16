#pragma once

#include <cstdint>
#include <string>

typedef uint8_t TeamID;

class Team {
public:
    TeamID id;
    std::string displayName;
private:

};