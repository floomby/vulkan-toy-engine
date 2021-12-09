#pragma once

#include <atomic>
#include <map>
#include <string>
#include <thread>
#include <queue>

#include "lua_wrapper.hpp"
#include "state.hpp"

#include <iostream>

// More api state stuff (needed for server -> client dependent callbacks)
namespace ApiUtil {
    static std::atomic<CallbackID> callbackCounter = 0;

    CallbackID getCallbackID();
    void doCallbackDispatch(CallbackID id, const ApiProtocol&);
    static std::map<CallbackID, const std::pair<const std::function<void (const ApiProtocol&)>, const LuaWrapper *>> callbacks;

    static std::multimap<const LuaWrapper *, const std::pair<std::function<void (const ApiProtocol&)>, const ApiProtocol>> otherThreadsData;

    extern thread_local std::queue<CallbackID> callbackIds;

    // TODO Fix this with template programming
    template<class T>
    T luaCallbackDispatcher(CallbackID id) {
        return T();
    }
    template<>
    inline std::function<void (unsigned)> luaCallbackDispatcher<std::function<void (unsigned)>>(CallbackID id) {
        return [id](unsigned x){
            LuaWrapper::threadLuaInstance->callCallback<unsigned>(id, x);
        };
    }
}