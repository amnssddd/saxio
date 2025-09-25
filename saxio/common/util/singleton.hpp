#pragma once

namespace saxio::util
{
template <class T>
class Singleton {
public:
    Singleton() = default;
    ~Singleton() = default;

public:
    static auto instance() -> T&{
        static T instance;
        return instance;
    }
};
}

