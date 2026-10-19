#pragma once

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "core/ForwardList.hpp"
#include "core/IStorage.hpp"
#include "core/IEnumerable.hpp"
#include "core/Text.hpp"
#include "pointers/PointerOperations.hpp"
#include "pointers/UniquePtr.hpp"
#include "sequences/DynamicArray.hpp"
#include "sequences/Sequence.hpp"

enum class NaryTraversal { KLP, KPL, LPK, PLK, BreadthFirst };

template <class T>
class NaryTree : public IStorage<T> {
private:
    struct Node {
        T value;
        DynamicArray<UniquePtr<Node>> children;
        Node* parent{nullptr};

        Node(T value, Node* parent = nullptr) : value(Move(value)), parent(parent) {}

        ~Node() {
            Node* node = this;
            while (node) {
                if (!node->children.IsEmpty()) node = node->children.Back().get();
                else {
                    if (node == this) break;
                    Node* parent = node->parent;
                    parent->children.PopBack();
                    node = parent;
                }
            }
        }
    };

    UniquePtr<Node> root;
    int arity;
    int count{0};
    int version{0};
    ForwardList<Node*> insertion;

    class Enumerator : public IEnumerator<T> {
    private:
        struct Frame { Node* node; int stage; };
        const NaryTree& owner;
        int version;
        NaryTraversal order;
        ForwardList<Frame> pending;
        Node* current{nullptr};

        void CheckVersion() const {
            if (version != owner.version) throw std::logic_error("Tree changed during enumeration.");
        }

    public:
        Enumerator(const NaryTree& owner, NaryTraversal order, Node* start)
            : owner(owner), version(owner.version), order(order) {
            if (static_cast<int>(order) < 0 || static_cast<int>(order) > 4) {
                throw std::invalid_argument("Invalid traversal.");
            }
            if (start) pending.PushFront(Frame{start, 0});
        }

        bool MoveNext() override {
            CheckVersion();
            current = nullptr;
            if (order == NaryTraversal::BreadthFirst) {
                if (pending.IsEmpty()) return false;
                current = (*pending.begin()).node;
                pending.PopFront();
                for (int i = 0; i < current->children.GetSize(); ++i) {
                    pending.PushBack(Frame{current->children.Get(i).get(), 0});
                }
                return true;
            }
            const bool postorder = order == NaryTraversal::LPK || order == NaryTraversal::PLK;
            const bool reversed = order == NaryTraversal::KPL || order == NaryTraversal::PLK;
            while (!pending.IsEmpty()) {
                Frame& frame = *pending.begin();
                if (!postorder && frame.stage == 0) {
                    ++frame.stage;
                    current = frame.node;
                    return true;
                }
                const int child = frame.stage - (postorder ? 0 : 1);
                const int children = frame.node->children.GetSize();
                if (child < children) {
                    ++frame.stage;
                    Node* next = frame.node->children.Get(reversed ? children - child - 1 : child).get();
                    pending.PushFront(Frame{next, 0});
                } else {
                    if (postorder) current = frame.node;
                    pending.PopFront();
                    if (current) return true;
                }
            }
            return false;
        }

        const T& CurrentRef() const override {
            CheckVersion();
            if (!current) throw std::logic_error("Enumerator has no current element.");
            return current->value;
        }

        Node* CurrentNode() const { return current; }
    };

    Node* FindNode(const T& value) const {
        Enumerator enumerator(*this, NaryTraversal::KLP, root.get());
        while (enumerator.MoveNext()) {
            if (enumerator.CurrentRef() == value) return enumerator.CurrentNode();
        }
        return nullptr;
    }

    static Node* Follow(Node* node, const Sequence<int>& path) {
        for (int i = 0; i < path.GetCount(); ++i) {
            if (!node) throw std::out_of_range("Path does not exist.");
            node = node->children.Get(path.Get(i)).get();
        }
        if (!node) throw std::out_of_range("Path does not exist.");
        return node;
    }

    void PrepareInsertion() {
        try {
            if (insertion.IsEmpty() && root) insertion.PushBack(root.get());
            while (!insertion.IsEmpty()) {
                Node* node = *insertion.begin();
                if (node->children.GetSize() < arity) return;
                for (int i = 0; i < node->children.GetSize(); ++i) insertion.PushBack(node->children.Get(i).get());
                insertion.PopFront();
            }
        } catch (...) {
            insertion.Clear();
            throw;
        }
    }

    void Erase(Node* node) {
        int removed = 0;
        Enumerator enumerator(*this, NaryTraversal::KLP, node);
        while (enumerator.MoveNext()) ++removed;
        insertion.Clear();
        if (!node->parent) root.reset();
        else {
            Node* parent = node->parent;
            int index = 0;
            while (parent->children.Get(index).get() != node) ++index;
            parent->children.RemoveAt(index);
        }
        count -= removed;
        ++version;
    }

    template <class Transform>
    static UniquePtr<Node> Clone(const Node* node, int& copied, Transform transform) {
        if (!node) return {};
        struct Frame { const Node* source; Node* target; };
        auto result = MakeUnique<Node>(transform(node->value));
        ForwardList<Frame> pending;
        pending.PushBack(Frame{node, result.get()});
        ++copied;
        while (!pending.IsEmpty()) {
            const Frame frame = *pending.begin();
            pending.PopFront();
            frame.target->children.Reserve(frame.source->children.GetSize());
            for (int i = 0; i < frame.source->children.GetSize(); ++i) {
                const Node* source = frame.source->children.Get(i).get();
                auto child = MakeUnique<Node>(transform(source->value), frame.target);
                pending.PushBack(Frame{source, child.get()});
                frame.target->children.Append(Move(child));
                ++copied;
            }
        }
        return result;
    }

    static bool Same(Node* first, Node* second) {
        struct Frame { Node* first; Node* second; };
        ForwardList<Frame> pending;
        pending.PushBack(Frame{first, second});
        while (!pending.IsEmpty()) {
            const Frame frame = *pending.begin();
            pending.PopFront();
            if (!(frame.first->value == frame.second->value)
                || frame.first->children.GetSize() != frame.second->children.GetSize()) return false;
            for (int i = 0; i < frame.first->children.GetSize(); ++i) {
                pending.PushBack(Frame{frame.first->children.Get(i).get(), frame.second->children.Get(i).get()});
            }
        }
        return true;
    }

public:
    NaryTree(int arity = 3) : arity(arity) {
        if (arity < 1) throw std::invalid_argument("Arity must be positive.");
    }

    NaryTree(const NaryTree& other) : arity(other.arity) {
        root = Clone(other.root.get(), count, [](const T& value) { return value; });
    }

    NaryTree(NaryTree&& other) noexcept
        : root(Move(other.root)), arity(other.arity), count(other.count), insertion(Move(other.insertion)) {
        other.count = 0;
        ++other.version;
    }

    NaryTree& operator=(const NaryTree& other) {
        if (this != &other) {
            NaryTree copy(other);
            Swap(copy);
        }
        return *this;
    }

    NaryTree& operator=(NaryTree&& other) noexcept {
        if (this != &other) {
            NaryTree moved(Move(other));
            Swap(moved);
        }
        return *this;
    }

    void Swap(NaryTree& other) noexcept {
        root.swap(other.root);
        ::Swap(arity, other.arity);
        ::Swap(count, other.count);
        insertion.Swap(other.insertion);
        ++version;
        ++other.version;
    }

    int GetCount() const override { return count; }
    int GetArity() const noexcept { return arity; }

    void Add(T value) {
        auto node = MakeUnique<Node>(Move(value));
        PrepareInsertion();
        if (!root) {
            insertion.PushBack(node.get());
            root = Move(node);
        } else {
            Node* parent = *insertion.begin();
            node->parent = parent;
            parent->children.Append(Move(node));
        }
        ++count;
        ++version;
    }

    void AddChild(const Sequence<int>& parentPath, T value) {
        Node* parent = Follow(root.get(), parentPath);
        if (parent->children.GetSize() == arity) throw std::invalid_argument("Node already has n children.");
        parent->children.Append(MakeUnique<Node>(Move(value), parent));
        insertion.Clear();
        ++count;
        ++version;
    }

    const T* Find(const T& value) const {
        const Node* node = FindNode(value);
        return node ? &node->value : nullptr;
    }

    bool Remove(const T& value) {
        Node* node = FindNode(value);
        if (!node) return false;
        Erase(node);
        return true;
    }

    void RemoveByPath(const Sequence<int>& path) { Erase(Follow(root.get(), path)); }

    void Clear() noexcept {
        insertion.Clear();
        root.reset();
        count = 0;
        ++version;
    }

    const T& GetByPath(const Sequence<int>& path) const { return Follow(root.get(), path)->value; }
    const T& GetByPath(const T& start, const Sequence<int>& path) const { return Follow(FindNode(start), path)->value; }

    UniquePtr<IEnumerator<T>> GetEnumerator() const override { return GetEnumerator(NaryTraversal::KLP); }

    UniquePtr<IEnumerator<T>> GetEnumerator(NaryTraversal order) const {
        return MakeUnique<Enumerator>(*this, order, root.get());
    }

    template <class Transform>
    NaryTree Map(Transform transform) const {
        NaryTree result(arity);
        result.root = Clone(root.get(), result.count, transform);
        return result;
    }

    template <class Predicate>
    NaryTree Where(Predicate predicate) const {
        NaryTree result(arity);
        auto enumerator = GetEnumerator(NaryTraversal::BreadthFirst);
        while (enumerator->MoveNext()) {
            const T& value = enumerator->CurrentRef();
            if (predicate(value)) result.Add(value);
        }
        return result;
    }

    template <class Accumulator, class Function>
    Accumulator Reduce(Accumulator initial, Function function, NaryTraversal order = NaryTraversal::KLP) const {
        auto enumerator = GetEnumerator(order);
        while (enumerator->MoveNext()) initial = function(enumerator->CurrentRef(), Move(initial));
        return initial;
    }

    NaryTree Merge(const IEnumerable<T>& other) const {
        NaryTree result(*this);
        auto enumerator = other.GetEnumerator();
        while (enumerator->MoveNext()) result.Add(enumerator->CurrentRef());
        return result;
    }

    NaryTree ExtractSubtree(const T& value) const {
        const Node* node = FindNode(value);
        if (!node) throw std::out_of_range("Subtree root not found.");
        NaryTree result(arity);
        result.root = Clone(node, result.count, [](const T& value) { return value; });
        return result;
    }

    bool ContainsSubtree(const NaryTree& other) const {
        if (!other.root) return true;
        Enumerator enumerator(*this, NaryTraversal::KLP, root.get());
        while (enumerator.MoveNext()) {
            if (Same(enumerator.CurrentNode(), other.root.get())) return true;
        }
        return false;
    }

    template <class Writer>
    std::string ToString(Writer write, NaryTraversal order = NaryTraversal::BreadthFirst) const {
        return WriteValues(GetEnumerator(order), write);
    }

    std::string ToString(NaryTraversal order = NaryTraversal::BreadthFirst) const {
        return ToString([](const T& value) { return WriteText(value); }, order);
    }

    template <class Reader>
    static NaryTree FromString(const std::string& text, int arity, Reader read) {
        NaryTree result(arity);
        std::istringstream input(text);
        std::string value;
        while (input >> std::ws && !input.eof()) {
            if (!(input >> std::quoted(value))) throw std::invalid_argument("Invalid value.");
            result.Add(read(value));
        }
        return result;
    }

    static NaryTree FromString(const std::string& text, int arity = 3) {
        return FromString(text, arity, [](const std::string& value) { return ReadText<T>(value); });
    }
};
