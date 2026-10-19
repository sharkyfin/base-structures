#pragma once

#include "core/IStorage.hpp"

template <class T>
class ISet : public IStorage<T> {
public:
    virtual bool Add(const T& value) = 0;
    virtual bool Remove(const T& value) = 0;
    virtual bool Contains(const T& value) const = 0;
    virtual UniquePtr<ISet<T>> Union(const ISet<T>& other) const = 0;
    virtual UniquePtr<ISet<T>> Intersection(const ISet<T>& other) const = 0;
    virtual UniquePtr<ISet<T>> Difference(const ISet<T>& other) const = 0;

    bool IsSubsetOf(const ISet<T>& other) const {
        if (this->GetCount() > other.GetCount()) return false;
        auto enumerator = this->GetEnumerator();
        while (enumerator->MoveNext()) {
            if (!other.Contains(enumerator->CurrentRef())) return false;
        }
        return true;
    }

    bool Equals(const ISet<T>& other) const {
        return this->GetCount() == other.GetCount() && IsSubsetOf(other);
    }
};
