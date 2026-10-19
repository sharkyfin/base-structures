#pragma once

#include <stdexcept>

#include "core/IPriorityStorage.hpp"
#include "heaps/BinaryHeap.hpp"
#include "pointers/UniquePtr.hpp"
#include "sequences/DynamicArray.hpp"
#include "trees/BinaryTree.hpp"

template <class T, class Compare = Less<T>>
class TreePriorityStorage : public IPriorityStorage<T> {
private:
    BinaryTree<T, Compare> tree;

public:
    TreePriorityStorage(Compare compare = Compare());

    int GetCount() const override;
    void Add(T value) override;
    const T& Peek() const override;
    T Pop() override;
    void Clear() override;
    bool IsValid() const override;
    UniquePtr<IEnumerator<T>> GetEnumerator() const override;
    UniquePtr<IPriorityStorage<T>> Clone() const override;
    UniquePtr<IPriorityStorage<T>> CreateEmpty() const override;
};

template <class T, class Compare = Less<T>>
class HeapPriorityStorage : public IPriorityStorage<T> {
private:
    BinaryHeap<T, Compare> heap;

    class Enumerator : public IEnumerator<T> {
    private:
        DynamicArray<const T*> values;
        int position{-1};

    public:
        Enumerator(const BinaryHeap<T, Compare>& heap);

        bool MoveNext() override;

        const T& CurrentRef() const override;
    };

public:
    HeapPriorityStorage(Compare compare = Compare());

    int GetCount() const override;
    void Add(T value) override;
    const T& Peek() const override;
    T Pop() override;
    void Clear() override;
    bool IsValid() const override;
    UniquePtr<IEnumerator<T>> GetEnumerator() const override;
    UniquePtr<IPriorityStorage<T>> Clone() const override;
    UniquePtr<IPriorityStorage<T>> CreateEmpty() const override;
};

#include "PriorityStorage.tpp"
