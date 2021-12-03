#pragma once

#include <vector>
#include <string>

template<typename E, E I> constexpr bool isValidEnumIndex() {
    auto name = __PRETTY_FUNCTION__;
    int i = strlen(name);
    for (; i >= 0; --i) {
        if (name[i] == ' ') return true;
        if (name[i] == ')') return false;
    }
    return false;
}

template<typename E, int I> constexpr int getEnumCount() {
    if constexpr (isValidEnumIndex<E, static_cast<E>(I)>()) return getEnumCount<E, I + 1>();
    else return I;
}

template<typename E> constexpr int getEnumCount() {
    return getEnumCount<E, 0>();
}

template<typename E, E V> constexpr std::string getEnumString() {
    auto name = __PRETTY_FUNCTION__;
    int i = strlen(name), j;
    for (int spaces = 0; i >= 0; --i) {
        if (name[i] == ' ') {
            spaces++;
            if (spaces == 3) j = i;
            if (spaces == 4) break;
        }
    }
    i++;
    std::string ret = "";
    for (; i < j-1; ++i) ret += name[i];
    return ret;
}

template<typename E, int val> constexpr std::vector<std::string> enumNames_i(std::vector<std::string> ret) {
    ret.push_back(getEnumString<E, E(val)>());
    if constexpr (val == 0)
        return ret;
    else
        return enumNames_i<E, val - 1>(ret);
}

// In reverse order
template<typename E> constexpr std::vector<std::string> enumNames() {
    std::vector<std::string> ret;
    return enumNames_i<E, static_cast<int>(getEnumCount<E>()) - 1>(ret);
}

template<typename E> constexpr std::vector<std::string> enumNames2() {
    auto ret = enumNames<E>();
    std::reverse(ret.begin(), ret.end());
    return ret;
}