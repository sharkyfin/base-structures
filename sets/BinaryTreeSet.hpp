#pragma once

#include "sets/ISet.hpp"
#include "trees/BinaryTree.hpp"

template <class T, class Compare = Less<T>>
class BinaryTreeSet : public ISet<T> {
private:
    enum class Operation { Union, Intersection, Difference };
    BinaryTree<T, Compare> tree;

    BinaryTreeSet Combine(const ISet<T>& other, Operation operation) const {
        BinaryTreeSet result(tree.GetCompare());
        auto enumerator = GetEnumerator();
        while (enumerator->MoveNext()) {
            const T& value = enumerator->CurrentRef();
            if (operation == Operation::Intersection && !other.Contains(value)) continue;
            if (operation == Operation::Difference && other.Contains(value)) continue;
            result.Add(value);
        }
        if (operation == Operation::Union) {
            enumerator = other.GetEnumerator();
            while (enumerator->MoveNext()) result.Add(enumerator->CurrentRef());
        }
        return result;
    }

public:
    BinaryTreeSet(Compare compare = Compare()) : tree(Move(compare)) {}

    int GetCount() const override { return tree.GetCount(); }
    bool Add(const T& value) override { return tree.Add(value); }
    bool Remove(const T& value) override { return tree.Remove(value); }
    bool Contains(const T& value) const override { return tree.Find(value) != nullptr; }
    bool IsValid() const { return tree.IsValid(); }
    int GetHeight() const { return tree.GetHeight(); }
    void Clear() { tree.Clear(); }

    UniquePtr<ISet<T>> Union(const ISet<T>& other) const override {
        return MakeUnique<BinaryTreeSet>(Combine(other, Operation::Union));
    }

    UniquePtr<ISet<T>> Intersection(const ISet<T>& other) const override {
        return MakeUnique<BinaryTreeSet>(Combine(other, Operation::Intersection));
    }

    UniquePtr<ISet<T>> Difference(const ISet<T>& other) const override {
        return MakeUnique<BinaryTreeSet>(Combine(other, Operation::Difference));
    }

    BinaryTreeSet operator|(const ISet<T>& other) const { return Combine(other, Operation::Union); }
    BinaryTreeSet operator&(const ISet<T>& other) const { return Combine(other, Operation::Intersection); }
    BinaryTreeSet operator-(const ISet<T>& other) const { return Combine(other, Operation::Difference); }
    bool operator<=(const ISet<T>& other) const { return this->IsSubsetOf(other); }
    bool operator==(const ISet<T>& other) const { return this->Equals(other); }
    bool operator!=(const ISet<T>& other) const { return !this->Equals(other); }

    template <class Transform>
    BinaryTreeSet Map(Transform transform) const {
        BinaryTreeSet result(tree.GetCompare());
        result.tree = tree.Map(transform);
        return result;
    }

    template <class Predicate>
    BinaryTreeSet Where(Predicate predicate) const {
        BinaryTreeSet result(tree.GetCompare());
        result.tree = tree.Where(predicate);
        return result;
    }

    template <class Accumulator, class Function>
    Accumulator Reduce(Accumulator initial, Function function) const {
        return tree.Reduce(Move(initial), function);
    }

    template <class Writer>
    std::string ToString(Writer write) const { return tree.ToString(write); }

    std::string ToString() const { return tree.ToString(); }

    template <class Reader>
    static BinaryTreeSet FromString(const std::string& text, Reader read, Compare compare = Compare()) {
        BinaryTreeSet result(compare);
        result.tree = BinaryTree<T, Compare>::FromString(text, read, Move(compare));
        return result;
    }

    static BinaryTreeSet FromString(const std::string& text, Compare compare = Compare()) {
        return FromString(text, [](const std::string& value) { return ReadText<T>(value); }, Move(compare));
    }

    UniquePtr<IEnumerator<T>> GetEnumerator() const override { return tree.GetEnumerator(); }
};
