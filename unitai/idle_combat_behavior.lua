local ran = false

function Idle_combat_behavior(instance)
    if ran then return end
    if eng_frame() % 30 == 0 then
        if (not engS_isEntityIdle(instance)) then return end
        local enemy = engS_getClosestEnemy(instance)
        if util_isNull(enemy) then return end
        -- print("running ai")
        -- io.flush()
        local pos = engS_getInstancePosition(instance)
        local epos = engS_getInstancePosition(enemy)
        local vec = epos - pos
        local dist = math.sqrt(vec * vec)
        local engret = engS_getState(instance, "engage")
        if dist > 18 and engret == IEngage__ENGAGE then
            local id = engS_getInstanceID(instance)
            local norm = vec / dist
            -- cmd_move(id, pos + norm * (dist - 17), InsertionMode__OVERWRITE)
            ran = true
        end
        if dist < 30 and engret == IEngage__AVOID then
            local id = engS_getInstanceID(instance)
            local norm = vec / dist
            -- cmd_move(id, pos - norm * (27 - dist), InsertionMode__OVERWRITE)
            ran = true
        end
    end
end