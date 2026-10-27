#pragma once

#include "core/ICollectible.hpp"

template <class T>
class IPriorityStorage : public ICollectible<T> {
public:
    virtual void Add(T value) = 0;
    virtual const T& Peek() const = 0;
    virtual T Pop() = 0;
    virtual void Clear() = 0;
    virtual bool IsValid() const = 0;
    virtual UniquePtr<IPriorityStorage<T>> Clone() const = 0;
    virtual UniquePtr<IPriorityStorage<T>> CreateEmpty() const = 0;
};
