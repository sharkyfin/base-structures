#pragma once

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "core/Pair.hpp"
#include "core/IEnumerable.hpp"
#include "core/IPriorityStorage.hpp"
#include "core/Text.hpp"
#include "queues/PriorityStorage.hpp"
#include "sequences/ArraySequence.hpp"

template <class T>
class PriorityQueue : public ICollectible<T> {
private:
    UniquePtr<IPriorityStorage<T>> storage;

public:
    PriorityQueue(UniquePtr<IPriorityStorage<T>> priorityStorage = MakeUnique<TreePriorityStorage<T>>());

    PriorityQueue(const PriorityQueue& other);

    PriorityQueue(PriorityQueue&& other);

    PriorityQueue& operator=(const PriorityQueue& other);

    PriorityQueue& operator=(PriorityQueue&& other);

    void Swap(PriorityQueue& other) noexcept;

    int GetCount() const override;
    bool IsValid() const;
    void Clear();
    void Enqueue(T value);
    const T& Peek() const;
    T Dequeue();

    bool Contains(const T& value) const;

    template <class Transform>
    PriorityQueue Map(Transform transform) const;

    template <class Predicate>
    PriorityQueue Where(Predicate predicate) const;

    template <class Accumulator, class Function>
    Accumulator Reduce(Accumulator initial, Function function) const;

    ArraySequence<T> GetSubsequence(int first, int last) const;

    bool ContainsSubsequence(const Sequence<T>& pattern) const;

    PriorityQueue Merge(const IEnumerable<T>& other) const;

    template <class Predicate>
    Pair<PriorityQueue, PriorityQueue> Split(Predicate predicate) const;

    template <class Writer>
    std::string ToString(Writer write) const;

    std::string ToString() const;

    template <class Reader>
    static PriorityQueue FromString(const std::string& text, Reader read,
        UniquePtr<IPriorityStorage<T>> priorityStorage);

    static PriorityQueue FromString(const std::string& text,
        UniquePtr<IPriorityStorage<T>> priorityStorage = MakeUnique<TreePriorityStorage<T>>());

    UniquePtr<IEnumerator<T>> GetEnumerator() const override;
};

#include "PriorityQueue.tpp"
