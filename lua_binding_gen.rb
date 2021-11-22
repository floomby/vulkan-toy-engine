#!/usr/bin/ruby

# finding class names and enum names automatically would be cool

# I am not sure that Instance * is ever ok to use in lua, there is a jank way to do it, but I don't think it seems worth it.
# Luckily (well by design actually) the instance ids are ordered in the auth state buffer and we can leverage that to do fast lookups

require 'pp'

require 'rbgccxml'

source = RbGCCXML.parse("api.hpp", :cxxflags => ["-std=c++20"])

class BindingGenerator
    @@enums = ["InsertionMode"]
    @@classes = ["Entity", "Instance"]
    @@classes_matchers = @@classes.map { |x| x + "*" }
    @@classes_regexes = @@classes.map { |x| /#{x} *\*/ }
    @@integers = ["uint32_t", "int", "unsigned int"]

    def initialize(name, rettype, argtypes)
        @name = name
        @rettype = rettype
        @argtypes = argtypes
        @static_name = @name.split("::")[-1]
    end

    def remove_std_allocators(typestr)
        if typestr.gsub!(/, ?std::allocator<[^>]*>/, "") != nil then
            remove_std_allocators(typestr)
        end
    end

    def stack_reader(argtype, i)
        acm = ""
        argtype.gsub!(/^\:\:/, "")
        case argtype
        when *@@integers
            return <<-END
    luaL_checkinteger(ls, #{i + 1});
    auto a#{i} = (#{argtype})lua_tointeger(ls, #{i + 1});
END
        when *@@enums
            return <<-END
    luaL_checkinteger(ls, #{i + 1});
    auto a#{i} = (#{argtype})lua_tointeger(ls, #{i + 1});
END
        when *["char*","std::string"]
            return <<-END
    luaL_checkstring(ls, #{i + 1});
    auto a#{i} = lua_tostring(ls, #{i + 1});
END
        when "glm::vec3"
            return <<-END
    if(!lua_istable(ls, #{i + 1})) throw std::runtime_error("Invalid lua arguments (table)");
    std::array<float, 3> v#{i};
    if (lua_objlen(ls, #{i + 1}) != 3) throw std::runtime_error("C++/Lua vector mismatch");
    for (int i = 1; i <= 3; i++) {
        lua_rawgeti(ls, #{i + 1}, i);
        luaL_checknumber(ls, -1);
        v#{i}[i - 1] = lua_tonumber(ls, -1);
        lua_pop(ls, 1);
    }
    #{argtype} a#{i}(v#{i}[0], v#{i}[1], v#{i}[2]);
END
        when "glm::quat"
            return <<-END
    if(!lua_istable(ls, #{i + 1})) throw std::runtime_error("Invalid lua arguments (table)");
    std::array<float, 4> v#{i};
    if (lua_objlen(ls, #{i + 1}) != 4) throw std::runtime_error("C++/Lua vector mismatch");
    for (int i = 1; i <= 4; i++) {
        lua_rawgeti(ls, #{i + 1}, i);
        luaL_checknumber(ls, -1);
        v#{i}[i - 1] = lua_tonumber(ls, -1);
        lua_pop(ls, 1);
    }
    #{argtype} a#{i}(v#{i}[0], v#{i}[1], v#{i}[2], v#{i}[3]);
END
        when *@@classes_matchers
            return <<-END
    if (!lua_islightuserdata(ls, #{i + 1})) throw std::runtime_error("Invalid lua arguments (pointer)");
    auto a#{i} = (#{argtype})lua_topointer(ls, #{i + 1});
END
        else
            puts "unsupported type: #{argtype} : #{i}"
        end
        return acm
    end

    def is_vector(typestr)
        return typestr.match?(/^[^<]*?vector<(.*)>/)
    end

    def vector_of(typestr)
        return typestr.match(/^[^<]*?vector<(.*)>/).captures[0]
    end

    def is_class_ptr(typestr)
        @@classes_regexes.each do |cm|
            if cm.match?(typestr); return true end
        end
        return false
    end

    # stuff that takes up one lua stack address (obviously this function is not complete yet)
    def basic_pusher(typestr)
        if is_class_ptr(typestr)
            return -> (x) { "lua_pushlightuserdata(ls, #{x});" }
        elsif @@integers.include?(typestr)
            return -> (x) { "lua_pushinteger(ls, #{x});" }
        else
            raise "Unsuported basic type #{typestr}"
        end
    end

    def lua_ret
        acm = ""
        remove_std_allocators(@rettype)
        if is_vector(@rettype)
            of_type = vector_of(@rettype)
            push = basic_pusher(of_type).call("r[i]")
            acm += <<-END
    lua_createtable(ls, r.size(), 0);
    for (int i = 0; i < r.size(); i++) {
        #{push}
        lua_rawseti(ls, -2, i + 1);
    }
END
        else
            acm += "    #{basic_pusher(@rettype).call("r")}"
        end
        return acm
    end

    def wrapper_gen
        acm = "static int #{@static_name}Wrapper(lua_State *ls) {\n"

        @argtypes.each_with_index do |at, i|
            at.gsub!(/ [a-zA-Z_][a-zA-Z_0-9]*$/, "")
            # We don't care if it is const
            at.gsub!(/(^const )|( const$)/, "")
            # We also don't care if it takes a reference
            at.gsub!(/\&$/, "")
            acm += stack_reader(at, i)
        end

        acm += "    #{@rettype == "void" ? "" : "auto r = "}" + @name.gsub!(/^\:\:/, "") + "("
        
        if @argtypes.length > 0 then
            acm += ((0..@argtypes.length - 1).to_a.map { |x| "a" + x.to_s }).join(", ")
        end
        acm += ");\n"
        if @rettype != "void" then
            acm += lua_ret
        end
        acm += "    return #{@rettype == "void" ? 0 : 1};\n}\n"

        return acm += "\n";
    end

    def exporter_gen
        acm = <<-END
static void #{@static_name}Export(lua_State *ls) {
    lua_pushcfunction(ls, #{@static_name}Wrapper);
    lua_setglobal(ls, "#{@static_name}");
}

END
    end

    def to_call
        return "    #{@static_name}Export(luaState);"
    end
end

puts <<-END
// This is an auto generated file changes can be overwritten durring build process
#include "../lua_wrapper.hpp"
#include "../api.hpp"

END

call_list = []

source.classes("Api").methods.each do |m|
    name = m.to_cpp
    rettype = m.return_type.to_cpp
    argtypes = m.arguments.map { |x| x.to_cpp }

    bg = BindingGenerator.new(name, rettype, argtypes)

    puts bg.wrapper_gen
    puts bg.exporter_gen
    call_list << bg.to_call
end

puts <<-END
void LuaWrapper::apiExport() {
#{call_list.join("\n")}
}
END