Vec3_mt = {
    __add = function(lhs, rhs)
        local ret = {}
        ret[1] = lhs[1] + rhs[1]
        ret[2] = lhs[2] + rhs[2]
        ret[3] = lhs[3] + rhs[3]
        return setmetatable(ret, Vec3_mt)
    end,

    __sub = function(lhs, rhs)
        local ret = {}
        ret[1] = lhs[1] - rhs[1]
        ret[2] = lhs[2] - rhs[2]
        ret[3] = lhs[3] - rhs[3]
        return setmetatable(ret, Vec3_mt)
    end,

    -- scalar product
    __mul = function(lhs, rhs)
        if type(lhs) == "number" then
            local ret = {}
            ret[1] = rhs[1] * lhs
            ret[2] = rhs[2] * lhs
            ret[3] = rhs[3] * lhs
            return setmetatable(ret, Vec3_mt)
        end
        if (type(rhs) == "table") then
            return lhs[1] * rhs[1] + lhs[2] * rhs[2] + lhs[3] * rhs[3]
        end
        local ret = {}
        ret[1] = lhs[1] * rhs
        ret[2] = lhs[2] * rhs
        ret[3] = lhs[3] * rhs
        return setmetatable(ret, Vec3_mt)
    end,

    __div = function(lhs, rhs)
        local ret = {}
        ret[1] = lhs[1] / rhs
        ret[2] = lhs[2] / rhs
        ret[3] = lhs[3] / rhs
        return setmetatable(ret, Vec3_mt)
    end,

    __tostring = function(arg)
        return "x: " .. arg[1] .. " y: " .. arg[2] .. " z: " .. arg[3]
    end,
}

Vec4_mt = {
    __add = function(lhs, rhs)
        local ret = {}
        ret[1] = lhs[1] + rhs[1]
        ret[2] = lhs[2] + rhs[2]
        ret[3] = lhs[3] + rhs[3]
        ret[4] = lhs[4] + rhs[4]
        return setmetatable(ret, Vec4_mt)
    end,

    __sub = function(lhs, rhs)
        local ret = {}
        ret[1] = lhs[1] - rhs[1]
        ret[2] = lhs[2] - rhs[2]
        ret[3] = lhs[3] - rhs[3]
        ret[4] = lhs[4] - rhs[4]
        return setmetatable(ret, Vec4_mt)
    end,

    -- scalar product
    __mul = function(lhs, rhs)
        if type(lhs) == "number" then
            local ret = {}
            ret[1] = rhs[1] * lhs
            ret[2] = rhs[2] * lhs
            ret[3] = rhs[3] * lhs
            ret[4] = rhs[4] * lhs
            return setmetatable(ret, Vec4_mt)
        end
        if type(rhs) == "table" then
            return lhs[1] * rhs[1] + lhs[2] * rhs[2] + lhs[3] * rhs[3] + lhs[4] * rhs[4]
        end
        local ret = {}
        ret[1] = lhs[1] * rhs
        ret[2] = lhs[2] * rhs
        ret[3] = lhs[3] * rhs
        ret[4] = lhs[4] * rhs
        return setmetatable(ret, Vec4_mt)
    end,

    __div = function(lhs, rhs)
        local ret = {}
        ret[1] = lhs[1] / rhs
        ret[2] = lhs[2] / rhs
        ret[3] = lhs[3] / rhs
        ret[4] = lhs[4] / rhs
        return setmetatable(ret, Vec4_mt)
    end,

    __tostring = function(arg)
        return "x: " .. arg[1] .. " y: " .. arg[2] .. " z: " .. arg[3] .. " w: " .. arg[4]
    end,
}

Quat_mt = {
    __add = function(lhs, rhs)
        local ret = {}
        ret[1] = lhs[1] + rhs[1]
        ret[2] = lhs[2] + rhs[2]
        ret[3] = lhs[3] + rhs[3]
        ret[4] = lhs[4] + rhs[4]
        return setmetatable(ret, Quat_mt)
    end,

    __sub = function(lhs, rhs)
        local ret = {}
        ret[1] = lhs[1] - rhs[1]
        ret[2] = lhs[2] - rhs[2]
        ret[3] = lhs[3] - rhs[3]
        ret[4] = lhs[4] - rhs[4]
        return setmetatable(ret, Quat_mt)
    end,

    __mul = function(lhs, rhs)
        if type(lhs) == "number" then
            local ret = {}
            ret[1] = rhs[1] * lhs
            ret[2] = rhs[2] * lhs
            ret[3] = rhs[3] * lhs
            ret[4] = rhs[4] * lhs
            return setmetatable(ret, Quat_mt)
        end
        if (type(rhs) == "table") then
            return math_multQuat(lhs, rhs)
        end
        local ret = {}
        ret[1] = lhs[1] * rhs
        ret[2] = lhs[2] * rhs
        ret[3] = lhs[3] * rhs
        ret[4] = lhs[4] * rhs
        return setmetatable(ret, Quat_mt)
    end,

    __div = function(lhs, rhs)
        local ret = {}
        ret[1] = lhs[1] / rhs
        ret[2] = lhs[2] / rhs
        ret[3] = lhs[3] / rhs
        ret[4] = lhs[4] / rhs
        return setmetatable(ret, Quat_mt)
    end,

    __tostring = function(arg)
        return "x: " .. arg[1] .. " y: " .. arg[2] .. " z: " .. arg[3] .. " w: " .. arg[4]
    end
}