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

    -- dot product
    __mul = function(lhs, rhs)
        return lhs[1] * rhs[1] + lhs[2] * rhs[2] + lhs[3] * rhs[3]
    end,

    __tostring = function(arg)
        return "x: " .. arg[1] .. " y: " .. arg[2] .. " z: " .. arg[3]
    end
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

    -- dot product
    __mul = function(lhs, rhs)
        return lhs[1] * rhs[1] + lhs[2] * rhs[2] + lhs[3] * rhs[3] + lhs[4] * rhs[4]
    end,

    __tostring = function(arg)
        return "x: " .. arg[1] .. " y: " .. arg[2] .. " z: " .. arg[3] .. " w: " .. arg[4]
    end
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
        return math_multQuat(lhs, rhs)
    end,

    __tostring = function(arg)
        return "x: " .. arg[1] .. " y: " .. arg[2] .. " z: " .. arg[3] .. " w: " .. arg[4]
    end
}