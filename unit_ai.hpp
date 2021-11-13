#pragma once

#include "entity.hpp"

class UnitAI {
public:
    UnitAI(Entity *whatFor);
    void runAI();
private:
    Entity *entity;
};