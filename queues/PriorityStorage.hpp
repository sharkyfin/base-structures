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
    TreePriorityStorage(Compare compare = Compare()) : tree(Move(compare), true) {}

    int GetCount() const override { return tree.GetCount(); }
    void Add(T value) override { tree.Add(Move(value)); }
    const T& Peek() const override { return tree.Maximum(); }
    T Pop() override { return tree.PopMaximum(); }
    void Clear() override { tree.Clear(); }
    bool IsValid() const override { return tree.IsValid(); }
    UniquePtr<IEnumerator<T>> GetEnumerator() const override { return tree.GetEnumerator(Traversal::PKL); }
    UniquePtr<IPriorityStorage<T>> Clone() const override { return MakeUnique<TreePriorityStorage>(*this); }
    UniquePtr<IPriorityStorage<T>> CreateEmpty() const override {
        return MakeUnique<TreePriorityStorage>(tree.GetCompare());
    }
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
        Enumerator(const BinaryHeap<T, Compare>& heap) {
            values.Reserve(heap.GetCount());
            auto enumerator = heap.GetEnumerator();
            while (enumerator->MoveNext()) values.Append(&enumerator->CurrentRef());
            values.Sort([&](const T* first, const T* second) { return heap.GetCompare()(*second, *first); });
        }

        bool MoveNext() override {
            if (position < values.GetSize()) ++position;
            return position < values.GetSize();
        }

        const T& CurrentRef() const override {
            if (position < 0 || position >= values.GetSize()) throw std::logic_error("Enumerator has no current element.");
            return *values.Get(position);
        }
    };

public:
    HeapPriorityStorage(Compare compare = Compare()) : heap(Move(compare)) {}

    int GetCount() const override { return heap.GetCount(); }
    void Add(T value) override { heap.Add(Move(value)); }
    const T& Peek() const override { return heap.Peek(); }
    T Pop() override { return heap.Pop(); }
    void Clear() override { heap.Clear(); }
    bool IsValid() const override { return heap.IsValid(); }
    UniquePtr<IEnumerator<T>> GetEnumerator() const override { return MakeUnique<Enumerator>(heap); }
    UniquePtr<IPriorityStorage<T>> Clone() const override { return MakeUnique<HeapPriorityStorage>(*this); }
    UniquePtr<IPriorityStorage<T>> CreateEmpty() const override {
        return MakeUnique<HeapPriorityStorage>(heap.GetCompare());
    }
};
