#pragma once

template <class T>
struct Less {
    constexpr bool operator()(const T& lhs, const T& rhs) const {
        return lhs < rhs;
    }
};
