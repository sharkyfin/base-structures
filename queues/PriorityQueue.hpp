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
class PriorityQueue : public IStorage<T> {
private:
    UniquePtr<IPriorityStorage<T>> storage;

public:
    PriorityQueue(UniquePtr<IPriorityStorage<T>> priorityStorage = MakeUnique<TreePriorityStorage<T>>())
        : storage(Move(priorityStorage)) {
        if (!storage) throw std::invalid_argument("Priority storage is required.");
    }

    PriorityQueue(const PriorityQueue& other) : storage(other.storage->Clone()) {}

    PriorityQueue(PriorityQueue&& other) : storage(other.storage->CreateEmpty()) {
        storage.swap(other.storage);
    }

    PriorityQueue& operator=(const PriorityQueue& other) {
        if (this != &other) {
            PriorityQueue copy(other);
            Swap(copy);
        }
        return *this;
    }

    PriorityQueue& operator=(PriorityQueue&& other) {
        if (this != &other) {
            PriorityQueue moved(Move(other));
            Swap(moved);
        }
        return *this;
    }

    void Swap(PriorityQueue& other) noexcept { storage.swap(other.storage); }

    int GetCount() const override { return storage->GetCount(); }
    bool IsValid() const { return storage->IsValid(); }
    void Clear() { storage->Clear(); }
    void Enqueue(T value) { storage->Add(Move(value)); }
    const T& Peek() const { return storage->Peek(); }
    T Dequeue() { return storage->Pop(); }

    bool Contains(const T& value) const {
        auto enumerator = GetEnumerator();
        while (enumerator->MoveNext()) {
            if (enumerator->CurrentRef() == value) return true;
        }
        return false;
    }

    template <class Transform>
    PriorityQueue Map(Transform transform) const {
        PriorityQueue result(storage->CreateEmpty());
        auto enumerator = GetEnumerator();
        while (enumerator->MoveNext()) result.Enqueue(transform(enumerator->CurrentRef()));
        return result;
    }

    template <class Predicate>
    PriorityQueue Where(Predicate predicate) const {
        PriorityQueue result(storage->CreateEmpty());
        auto enumerator = GetEnumerator();
        while (enumerator->MoveNext()) {
            const T& value = enumerator->CurrentRef();
            if (predicate(value)) result.Enqueue(value);
        }
        return result;
    }

    template <class Accumulator, class Function>
    Accumulator Reduce(Accumulator initial, Function function) const {
        auto enumerator = GetEnumerator();
        while (enumerator->MoveNext()) initial = function(enumerator->CurrentRef(), Move(initial));
        return initial;
    }

    ArraySequence<T> GetSubsequence(int first, int last) const {
        if (first < 0 || last < first || last >= GetCount()) throw std::out_of_range("Invalid queue range.");
        ArraySequence<T> result;
        result.Reserve(last - first + 1);
        auto enumerator = GetEnumerator();
        for (int i = 0; i <= last; ++i) {
            enumerator->MoveNext();
            if (i >= first) result.Append(enumerator->CurrentRef());
        }
        return result;
    }

    bool ContainsSubsequence(const Sequence<T>& pattern) const {
        const int length = pattern.GetCount();
        if (length == 0) return true;
        if (length > GetCount()) return false;
        DynamicArray<int> prefix;
        prefix.Resize(length, 0);
        for (int i = 1, matched = 0; i < length; ++i) {
            while (matched > 0 && !(pattern.Get(i) == pattern.Get(matched))) matched = prefix[matched - 1];
            if (pattern.Get(i) == pattern.Get(matched)) ++matched;
            prefix[i] = matched;
        }
        auto enumerator = GetEnumerator();
        int matched = 0;
        while (enumerator->MoveNext()) {
            const T& value = enumerator->CurrentRef();
            while (matched > 0 && !(value == pattern.Get(matched))) matched = prefix[matched - 1];
            if (value == pattern.Get(matched)) ++matched;
            if (matched == length) return true;
        }
        return false;
    }

    PriorityQueue Merge(const IEnumerable<T>& other) const {
        PriorityQueue result(*this);
        auto enumerator = other.GetEnumerator();
        while (enumerator->MoveNext()) result.Enqueue(enumerator->CurrentRef());
        return result;
    }

    template <class Predicate>
    Pair<PriorityQueue, PriorityQueue> Split(Predicate predicate) const {
        Pair<PriorityQueue, PriorityQueue> result{PriorityQueue(storage->CreateEmpty()), PriorityQueue(storage->CreateEmpty())};
        auto enumerator = GetEnumerator();
        while (enumerator->MoveNext()) {
            const T& value = enumerator->CurrentRef();
            if (predicate(value)) result.key.Enqueue(value);
            else result.value.Enqueue(value);
        }
        return result;
    }

    template <class Writer>
    std::string ToString(Writer write) const { return WriteValues(GetEnumerator(), write); }

    std::string ToString() const { return ToString([](const T& value) { return WriteText(value); }); }

    template <class Reader>
    static PriorityQueue FromString(const std::string& text, Reader read,
        UniquePtr<IPriorityStorage<T>> priorityStorage) {
        PriorityQueue result(Move(priorityStorage));
        result.Clear();
        std::istringstream input(text);
        std::string value;
        while (input >> std::ws && !input.eof()) {
            if (!(input >> std::quoted(value))) throw std::invalid_argument("Invalid value.");
            result.Enqueue(read(value));
        }
        return result;
    }

    static PriorityQueue FromString(const std::string& text,
        UniquePtr<IPriorityStorage<T>> priorityStorage = MakeUnique<TreePriorityStorage<T>>()) {
        return FromString(text, [](const std::string& value) { return ReadText<T>(value); }, Move(priorityStorage));
    }

    UniquePtr<IEnumerator<T>> GetEnumerator() const override { return storage->GetEnumerator(); }
};
