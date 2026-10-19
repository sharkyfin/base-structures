#pragma once

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "core/ForwardList.hpp"
#include "core/Functional.hpp"
#include "core/IStorage.hpp"
#include "core/IEnumerable.hpp"
#include "core/Text.hpp"

enum class Traversal { KLP, KPL, LPK, LKP, PLK, PKL, BreadthFirst };

template <class T, class Compare = Less<T>>
class BinaryTree : public IStorage<T> {
private:
    struct Node {
        T value;
        UniquePtr<Node> left;
        UniquePtr<Node> right;
        int height{1};

        Node(T val) : value(Move(val)) {}
    };

    UniquePtr<Node> root;
    int count{0};
    Compare compare;
    bool allowDuplicates{false};
    int version{0};

    static int Height(const UniquePtr<Node>& node) noexcept {
        return node ? node->height : 0;
    }

    static void Update(Node& node) noexcept {
        const int lh = Height(node.left);
        const int rh = Height(node.right);
        node.height = 1 + (lh > rh ? lh : rh);
    }

    static void RotateLeft(UniquePtr<Node>& node) {
        auto next = Move(node->right);
        node->right = Move(next->left);
        Update(*node);
        next->left = Move(node);
        Update(*next);
        node = Move(next);
    }

    static void RotateRight(UniquePtr<Node>& node) {
        auto next = Move(node->left);
        node->left = Move(next->right);
        Update(*node);
        next->right = Move(node);
        Update(*next);
        node = Move(next);
    }

    static void Balance(UniquePtr<Node>& node) {
        if (!node) return;
        Update(*node);
        const int difference = Height(node->left) - Height(node->right);
        if (difference > 1) {
            if (Height(node->left->left) < Height(node->left->right)) RotateLeft(node->left);
            RotateRight(node);
        } else if (difference < -1) {
            if (Height(node->right->right) < Height(node->right->left)) RotateRight(node->right);
            RotateLeft(node);
        }
    }

    void Invalidate() noexcept {
        ++version;
    }

    template <class U>
    bool Insert(UniquePtr<Node>& node, U&& value) {
        if (!node) {
            node = MakeUnique<Node>(static_cast<U&&>(value));
            return true;
        }
        bool inserted = false;
        if (compare(value, node->value)) inserted = Insert(node->left, static_cast<U&&>(value));
        else if (allowDuplicates || compare(node->value, value)) {
            inserted = Insert(node->right, static_cast<U&&>(value));
        } else return false;
        Balance(node);
        return inserted;
    }

    static UniquePtr<Node> ExtractMinimum(UniquePtr<Node>& node) {
        if (!node->left) {
            auto minimum = Move(node);
            node = Move(minimum->right);
            return minimum;
        }
        auto minimum = ExtractMinimum(node->left);
        Balance(node);
        return minimum;
    }

    static UniquePtr<Node> ExtractMaximum(UniquePtr<Node>& node) {
        if (!node->right) {
            auto maximum = Move(node);
            node = Move(maximum->left);
            return maximum;
        }
        auto maximum = ExtractMaximum(node->right);
        Balance(node);
        return maximum;
    }

    template <class Key>
    bool Erase(UniquePtr<Node>& node, const Key& key) {
        if (!node) return false;
        bool removed = true;
        if (compare(key, node->value)) removed = Erase(node->left, key);
        else if (compare(node->value, key)) removed = Erase(node->right, key);
        else if (!node->left) {
            auto next = Move(node->right);
            node = Move(next);
        } else if (!node->right) {
            auto next = Move(node->left);
            node = Move(next);
        } else {
            auto next = ExtractMinimum(node->right);
            next->left = Move(node->left);
            next->right = Move(node->right);
            node = Move(next);
        }
        Balance(node);
        return removed;
    }

    template <class Key>
    const Node* FindNode(const Key& key) const {
        const Node* node = root.get();
        while (node) {
            if (compare(key, node->value)) node = node->left.get();
            else if (compare(node->value, key)) node = node->right.get();
            else return node;
        }
        return nullptr;
    }

    static UniquePtr<Node> Clone(const Node* node, int& copied) {
        if (!node) return {};
        auto result = MakeUnique<Node>(node->value);
        result->left = Clone(node->left.get(), copied);
        result->right = Clone(node->right.get(), copied);
        result->height = node->height;
        ++copied;
        return result;
    }

    int Check(const Node* node, const T* lower, const T* upper, int& visited) const {
        if (!node) return 0;
        if (lower && (compare(node->value, *lower) || (!allowDuplicates && !compare(*lower, node->value)))) return -1;
        if (upper && (compare(*upper, node->value) || (!allowDuplicates && !compare(node->value, *upper)))) return -1;
        ++visited;
        const int left = Check(node->left.get(), lower, &node->value, visited);
        const int right = Check(node->right.get(), &node->value, upper, visited);
        const int diff = left > right ? left - right : right - left;
        const int maxH = left > right ? left : right;
        if (left < 0 || right < 0 || diff > 1 || node->height != 1 + maxH) return -1;
        return node->height;
    }

    bool Same(const Node* first, const Node* second) const {
        if (!first || !second) return first == second;
        return !compare(first->value, second->value) && !compare(second->value, first->value)
            && Same(first->left.get(), second->left.get()) && Same(first->right.get(), second->right.get());
    }

    bool ContainsNode(const Node* node, const Node* candidate) const {
        if (!candidate) return true;
        if (!node) return false;
        if (compare(candidate->value, node->value)) return ContainsNode(node->left.get(), candidate);
        if (compare(node->value, candidate->value)) return ContainsNode(node->right.get(), candidate);
        return Same(node, candidate) || (allowDuplicates
            && (ContainsNode(node->left.get(), candidate) || ContainsNode(node->right.get(), candidate)));
    }

    static const T& Follow(const Node* node, const std::string& path) {
        for (char step : path) {
            if (step != 'L' && step != 'R') throw std::invalid_argument("Use L and R in paths.");
            if (!node) throw std::out_of_range("Path does not exist.");
            node = step == 'L' ? node->left.get() : node->right.get();
        }
        if (!node) throw std::out_of_range("Path does not exist.");
        return node->value;
    }

    class Enumerator : public IEnumerator<T> {
    private:
        struct Frame { const Node* node; int stage; };
        const BinaryTree& owner;
        int version;
        Traversal order;
        ForwardList<Frame> pending;
        const Node* current{nullptr};

        void CheckVersion() const {
            if (version != owner.version) throw std::logic_error("Tree changed during enumeration.");
        }

    public:
        Enumerator(const BinaryTree& owner, Traversal order)
            : owner(owner), version(owner.version), order(order) {
            if (owner.root) pending.PushFront(Frame{owner.root.get(), 0});
            if (static_cast<int>(order) < 0 || static_cast<int>(order) > 6) {
                throw std::invalid_argument("Invalid traversal.");
            }
        }

        bool MoveNext() override {
            CheckVersion();
            current = nullptr;
            if (order == Traversal::BreadthFirst) {
                if (pending.IsEmpty()) return false;
                current = (*pending.begin()).node;
                pending.PopFront();
                if (current->left) pending.PushBack(Frame{current->left.get(), 0});
                if (current->right) pending.PushBack(Frame{current->right.get(), 0});
                return true;
            }
            const char* steps[]{"KLR", "KRL", "LRK", "LKR", "RLK", "RKL"};
            while (!pending.IsEmpty()) {
                Frame& frame = *pending.begin();
                if (frame.stage == 3) { pending.PopFront(); continue; }
                const char step = steps[static_cast<int>(order)][frame.stage++];
                if (step == 'K') { current = frame.node; return true; }
                const Node* child = step == 'L' ? frame.node->left.get() : frame.node->right.get();
                if (child) pending.PushFront(Frame{child, 0});
            }
            return false;
        }

        const T& CurrentRef() const override {
            CheckVersion();
            if (!current) throw std::logic_error("Enumerator has no current element.");
            return current->value;
        }
    };

public:
    BinaryTree(Compare compare = Compare(), bool allowDuplicates = false)
        : compare(Move(compare)), allowDuplicates(allowDuplicates) {}

    BinaryTree(const BinaryTree& other)
        : compare(other.compare), allowDuplicates(other.allowDuplicates) {
        root = Clone(other.root.get(), count);
    }

    BinaryTree(BinaryTree&& other)
        : root(Move(other.root)), count(other.count), compare(Move(other.compare)), allowDuplicates(other.allowDuplicates) {
        other.count = 0;
        other.Invalidate();
    }

    BinaryTree& operator=(const BinaryTree& other) {
        if (this != &other) {
            BinaryTree copy(other);
            Swap(copy);
        }
        return *this;
    }

    BinaryTree& operator=(BinaryTree&& other) {
        if (this != &other) {
            BinaryTree moved(Move(other));
            Swap(moved);
        }
        return *this;
    }

    void Swap(BinaryTree& other) {
        ::Swap(compare, other.compare);
        root.swap(other.root);
        ::Swap(count, other.count);
        ::Swap(allowDuplicates, other.allowDuplicates);
        Invalidate();
        other.Invalidate();
    }

    int GetCount() const override { return count; }
    int GetHeight() const noexcept { return Height(root); }
    const Compare& GetCompare() const noexcept { return compare; }

    bool IsValid() const {
        int visited = 0;
        return Check(root.get(), nullptr, nullptr, visited) >= 0 && visited == count;
    }

    template <class U>
    bool Add(U&& value) {
        if (!Insert(root, static_cast<U&&>(value))) return false;
        ++count;
        Invalidate();
        return true;
    }

    template <class Key>
    bool Remove(const Key& key) {
        if (!Erase(root, key)) return false;
        --count;
        Invalidate();
        return true;
    }

    template <class Key>
    const T* Find(const Key& key) const {
        const Node* node = FindNode(key);
        return node ? &node->value : nullptr;
    }

    void Clear() noexcept {
        Invalidate();
        root.reset();
        count = 0;
    }

    const T& Maximum() const {
        if (!root) throw std::out_of_range("Tree is empty.");
        const Node* node = root.get();
        while (node->right) node = node->right.get();
        return node->value;
    }

    T PopMaximum() {
        if (!root) throw std::out_of_range("Tree is empty.");
        auto maximum = ExtractMaximum(root);
        --count;
        Invalidate();
        return Move(maximum->value);
    }

    UniquePtr<IEnumerator<T>> GetEnumerator() const override {
        return GetEnumerator(Traversal::LKP);
    }

    UniquePtr<IEnumerator<T>> GetEnumerator(Traversal order) const {
        return MakeUnique<Enumerator>(*this, order);
    }

    template <class Transform>
    BinaryTree Map(Transform transform, Traversal order = Traversal::LKP) const {
        BinaryTree result(compare, allowDuplicates);
        auto enumerator = GetEnumerator(order);
        while (enumerator->MoveNext()) result.Add(transform(enumerator->CurrentRef()));
        return result;
    }

    template <class Predicate>
    BinaryTree Where(Predicate predicate, Traversal order = Traversal::LKP) const {
        BinaryTree result(compare, allowDuplicates);
        auto enumerator = GetEnumerator(order);
        while (enumerator->MoveNext()) {
            const T& value = enumerator->CurrentRef();
            if (predicate(value)) result.Add(value);
        }
        return result;
    }

    template <class Accumulator, class Function>
    Accumulator Reduce(Accumulator initial, Function function, Traversal order = Traversal::LKP) const {
        auto enumerator = GetEnumerator(order);
        while (enumerator->MoveNext()) initial = function(enumerator->CurrentRef(), Move(initial));
        return initial;
    }

    BinaryTree Merge(const IEnumerable<T>& other) const {
        BinaryTree result(*this);
        auto enumerator = other.GetEnumerator();
        while (enumerator->MoveNext()) result.Add(enumerator->CurrentRef());
        return result;
    }

    BinaryTree ExtractSubtree(const T& value) const {
        const Node* node = FindNode(value);
        if (!node) throw std::out_of_range("Subtree root not found.");
        BinaryTree result(compare, allowDuplicates);
        result.root = Clone(node, result.count);
        return result;
    }

    bool ContainsSubtree(const BinaryTree& other) const {
        return ContainsNode(root.get(), other.root.get());
    }

    const T& GetByPath(const std::string& path) const { return Follow(root.get(), path); }
    const T& GetByPath(const T& start, const std::string& path) const { return Follow(FindNode(start), path); }

    template <class Writer>
    std::string ToString(Writer write) const {
        return WriteValues(GetEnumerator(), write);
    }

    std::string ToString() const {
        return ToString([](const T& value) { return WriteText(value); });
    }

    template <class Reader>
    static BinaryTree FromString(const std::string& text, Reader read,
        Compare compare = Compare(), bool allowDuplicates = false) {
        BinaryTree result(Move(compare), allowDuplicates);
        std::istringstream input(text);
        std::string value;
        while (input >> std::ws && !input.eof()) {
            if (!(input >> std::quoted(value))) throw std::invalid_argument("Invalid value.");
            result.Add(read(value));
        }
        return result;
    }

    static BinaryTree FromString(const std::string& text, Compare compare = Compare(), bool allowDuplicates = false) {
        return FromString(text, [](const std::string& value) { return ReadText<T>(value); }, Move(compare), allowDuplicates);
    }
};
