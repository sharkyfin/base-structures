#pragma once

#include "sequences/ArraySequence.hpp"

template <class T, class Compare = Less<T>>
class BinaryHeap : public IStorage<T> {
private:
    ArraySequence<T> values;
    Compare compare;

    void Up(int index) {
        while (index > 0) {
            const int parent = (index - 1) / 2;
            if (!compare(values.Get(parent), values.Get(index))) break;
            Swap(values.GetMutable(parent), values.GetMutable(index));
            index = parent;
        }
    }

    void Down(int index) {
        const int count = GetCount();
        while (index < count / 2) {
            int child = index * 2 + 1;
            if (child + 1 < count && compare(values.Get(child), values.Get(child + 1))) ++child;
            if (!compare(values.Get(index), values.Get(child))) break;
            Swap(values.GetMutable(index), values.GetMutable(child));
            index = child;
        }
    }

    void EraseAt(int index) {
        const int last = GetCount() - 1;
        if (index != last) values.GetMutable(index) = Move(values.GetMutable(last));
        values.RemoveAt(last);
        if (index < GetCount()) {
            if (index > 0 && compare(values.Get((index - 1) / 2), values.Get(index))) Up(index);
            else Down(index);
        }
    }

public:
    BinaryHeap(Compare compare = Compare()) : compare(Move(compare)) {}

    BinaryHeap(const IStorage<T>& source, Compare compare = Compare())
        : values(source), compare(Move(compare)) {
        for (int i = GetCount() / 2; i > 0; --i) Down(i - 1);
    }

    int GetCount() const override { return values.GetCount(); }
    const Compare& GetCompare() const noexcept { return compare; }
    void Clear() { values.Clear(); }

    void Add(T value) {
        values.Append(Move(value));
        Up(GetCount() - 1);
    }

    const T& Peek() const { return values.GetFirst(); }

    T Pop() {
        T result = Peek();
        EraseAt(0);
        return result;
    }

    bool Contains(const T& value) const {
        for (int i = 0; i < GetCount(); ++i) {
            if (values.Get(i) == value) return true;
        }
        return false;
    }

    bool Remove(const T& value) {
        for (int i = 0; i < GetCount(); ++i) {
            if (values.Get(i) == value) { EraseAt(i); return true; }
        }
        return false;
    }

    bool IsValid() const {
        for (int i = 1; i < GetCount(); ++i) {
            if (compare(values.Get((i - 1) / 2), values.Get(i))) return false;
        }
        return true;
    }

    UniquePtr<IEnumerator<T>> GetEnumerator() const override { return values.GetEnumerator(); }
};
