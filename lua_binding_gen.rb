#!/usr/bin/ruby

# finding class names and enum names automatically would be cool

# I am not sure that Instance * is ever ok to use in lua, there is a jank way to do it, but I don't think it seems worth it.
# Luckily (well by design actually) the instance ids are ordered in the auth state buffer and we can leverage that to do fast lookups

require 'pp'

require 'rbgccxml'

source = RbGCCXML.parse("api.hpp", :cxxflags => ["-std=c++20"])

class String
    def valid_brackets?
        valid = true
        self.gsub(/[^<>]/, '').split('').inject(0) do |counter, bracket|
            counter += (bracket == '<' ? 1 : -1)
            valid = false if counter < 0
            counter
        end.zero? && valid
    end
end

class BindingGenerator
    @@enums = ["InsertionMode", "IEngage"]
    @@classes = ["Entity", "Instance"]
    @@classes_matchers = @@classes.map { |x| x + "*" }
    @@classes_regexes = @@classes.map { |x| /#{x} *\*/ }
    @@integers = ["uint32_t", "int", "unsigned int", "InstanceID", "TeamID", "unsigned char"]
    @@floats = ["float", "double"]
    @@bool = ["bool"]

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
    auto a#{i} = (#{argtype})luaL_checkinteger(ls, #{i + 1});
END
        when *@@floats
            return <<-END
    auto a#{i} = (#{argtype})luaL_checknumber(ls, #{i + 1});
END
        when *@@enums
            return <<-END
    auto a#{i} = (#{argtype})luaL_checkinteger(ls, #{i + 1});
END
        when *@@bool
            return <<-END
    auto a#{i} = lua_toboolean(ls, #{i + 1});
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
        when /^std::function</
            cbtype = argtype.clone
            argtype.gsub!(/^std::function</, "")
            argtype.gsub!(/>/, "")
            if argtype.gsub!(/^void /) == nil
                puts "Non void returning callbacks are nonsense"
            end
            # This supports multiple callbacks, the networking protocol does not, but can be made to easily
            return <<-END
    CallbackID id#{i} = 0;
    if (!lua_isnil(ls, -1)) {
        id#{i} = ApiUtil::getCallbackID();
        lua_getglobal(ls, "Server_callbacks");
        if (!lua_istable(ls, -1)) throw std::runtime_error("Server_callbacks should be a table (did you forget to enable callbacks on this thread?)");
        lua_insert(ls, -2);
        lua_pushinteger(ls, id#{i});
        lua_insert(ls, -2);
        lua_settable(ls, -3);
    }
    lua_pop(ls, 1);
    ApiUtil::callbackIds.push(id#{i});
    auto a#{i} = ApiUtil::luaCallbackDispatcher<#{cbtype}>(id#{i});
END
        else
            puts "unsupported type: #{argtype} : #{i}"
            raise
        end
        return acm
    end

    def is_vector(typestr)
        return typestr.match?(/^[^<]*?vector<(.*)>/)
    end

    def is_pair(typestr)
        return typestr.match?(/^[^<]*?pair<(.*)>/)
    end

    def vector_of(typestr)
        return typestr.match(/^[^<]*?vector<(.*)>/).captures[0]
    end

    def get_type(ts)
        tmp = ts.split(',')
        i = 0   
        while true
            str = tmp[0..i].join(',')
            i += 1
            if str.valid_brackets? then
                return str
            end
        end
    end

    def pair_of(typestr)
        ts = typestr.clone
        ts.gsub!(/^[^<]*?pair</, "")
        ts.gsub!(/>$/, "")
        t1 = get_type(ts)
        ts = ts[t1.length..-1]
        ts.gsub!(/^, */, "")
        t2 = get_type(ts)
        return [t1, t2]
    end

    def is_class_ptr(typestr)
        @@classes_regexes.each do |cm|
            if cm.match?(typestr); return true end
        end
        return false
    end

    # stuff that takes up one lua stack address (obviously this function is not complete yet)
    def basic_pusher(typestr)
        remove_refs_and_const(typestr)
        typestr.gsub!(/^::/, "")
        if is_class_ptr(typestr)
            return -> (x) { "lua_pushlightuserdata(ls, #{x});" }
        elsif @@integers.include?(typestr)
            return -> (x) { "lua_pushinteger(ls, #{x});" }
        elsif @@floats.include?(typestr)
            return -> (x) { "lua_pushnumber(ls, #{x});" }
        elsif @@bool.include?(typestr)
            return -> (x) { "lua_pushboolean(ls, #{x});"}
        # oops
        elsif typestr == "std::string" || typestr == "std::basic_string<char, std::char_traits<char>>>>" || typestr == "std::basic_string<char, std::char_traits<char>>" 
            return -> (x) { "lua_pushstring(ls, #{x}.c_str());" }
        else
            raise "Unsuported basic type #{typestr}"
        end
    end

    def lua_ret
        @retcount = 1
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
        elsif is_pair(@rettype)
            of_types = pair_of(@rettype)
            p1 = basic_pusher(of_types[0]).call("r.first")
            p2 = basic_pusher(of_types[1]).call("r.second")
            acm += <<-END
    #{p1}
    #{p2}
END
            @retcount = 2
        else
            acm += "    #{basic_pusher(@rettype).call("r")}"
        end
        return acm
    end

    def remove_refs_and_const(x)
        x.gsub!(/(^const )|( const$)/, "")
        x.gsub!(/\&$/, "")
    end

    def wrapper_gen
        acm = "static int #{@static_name}Wrapper(lua_State *ls) {\n"

        @argtypes.each_with_index do |at, i|
            at.gsub!(/ [a-zA-Z_][a-zA-Z_0-9]*$/, "")
            remove_refs_and_const(at)
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
        acm += "    return #{@rettype == "void" ? 0 : @retcount};\n}\n"

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
#include "../api_util.hpp"

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