#pragma once

#include <atomic>
#include <map>
#include <string>
#include <thread>

#include "lua_wrapper.hpp"

#include <iostream>

// More api state stuff (needed for server -> client dependent callbacks)
namespace ApiUtil {
    static std::string name;
    static std::atomic<unsigned long> callbackCounter = 0;

    static std::map<unsigned long, std::pair<std::thread::id, void (*)()>> callbacks;
    unsigned long getCallbackID();

    // TODO Fix this with template programming
    template<class T>
    T luaCallbackDispatcher(unsigned long id) {
        return T();
    }
    template<>
    inline std::function<void (unsigned)> luaCallbackDispatcher<std::function<void (unsigned)>>(unsigned long id) {
        return [id](unsigned x){
            LuaWrapper::threadLuaInstance->callCallback<unsigned>(id, x);
        };
    }
}