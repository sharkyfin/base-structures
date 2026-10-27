#pragma once

#include "sequences/ArraySequence.hpp"

template <class T, class Compare = Less<T>>
class BinaryHeap : public ICollectible<T> {
private:
    ArraySequence<T> values;
    Compare compare;

    void Up(int index);

    void Down(int index);

    void EraseAt(int index);

public:
    BinaryHeap(Compare compare = Compare());

    BinaryHeap(const ICollectible<T>& source, Compare compare = Compare());

    int GetCount() const override;
    const Compare& GetCompare() const noexcept;
    void Clear();

    void Add(T value);

    const T& Peek() const;

    T Pop();

    bool Contains(const T& value) const;

    bool Remove(const T& value);

    bool IsValid() const;

    UniquePtr<IEnumerator<T>> GetEnumerator() const override;
};

#include "BinaryHeap.tpp"
