function Idle_combat_behavior(instance)
    if eng_frame() % 30 == 0 then
        local enemy = engS_getClosestEnemy(instance)
        if util_isNull(enemy) then return end
        local pos = engS_getInstancePosition(instance)
        local epos = engS_getInstancePosition(enemy)
        local test = pos + epos
        print(engS_getInstanceHeading(instance))
    end
end