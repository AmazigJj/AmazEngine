#pragma once

#include <type_traits>
#include <functional>

// Inspired by https://artificial-mind.net/blog/2020/10/24/range_ref

namespace amaz::util {

// a non-owning, lightweight view of a range
// whose element types are convertible to T
template <class T>
struct range_view {
    // iterates over the viewed range and invokes callback for each element
    void for_each(std::function<void(T)> callback) { 
        _for_each(_range, callback);
    }

    // empty range
    range_view() {
        _for_each = [](void*, std::function<void(T)>) {};
    }

    // any compatible range
	template <std::ranges::range Range>
    range_view(Range&& range) {
        _range = const_cast<void*>(static_cast<const void*>(&range));
        _for_each = [](void* r, std::function<void(T)> callback) {
            for (auto&& v : *reinterpret_cast<decltype(&range)>(r))
                callback(v);
        };
    }

    // {initializer, list, syntax}
    template <std::convertible_to<T> U>
    range_view(std::initializer_list<U>& range) {
        _range = const_cast<void*>(static_cast<const void*>(&range));
        _for_each = [](void* r, std::function<void(T)> f) {
            for (auto&& v : *static_cast<decltype(&range)>(r))
                f(v);
        };
    }

private:
	using range_func_t = std::function<void(void*, std::function<void(T)>)>; // void (*)(void*, std::function<void(T)>);

    void* _range = nullptr;
    range_func_t _for_each = nullptr;
};

}