#pragma once

#include "core/IEnumerable.hpp"

template <class T>
class IStorage : public IEnumerable<T> {
public:
    ~IStorage() override = default;
    virtual int GetCount() const = 0;

    bool IsEmpty() const {
        return GetCount() == 0;
    }
};
