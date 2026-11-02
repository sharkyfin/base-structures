#pragma once

#include <string>

#include "sequences/ArraySequence.hpp"
#include "trees/BinaryTree.hpp"

struct TreeNodeView {
    int value{0};
    int left{0};
    int right{0};
    bool hasLeft{false};
    bool hasRight{false};
    int height{1};
    int balance{0};
};

struct TreeFrame {
    ArraySequence<TreeNodeView> nodes;
    ArraySequence<int> visited;
    std::string action;
    std::string message;
    int current{0};
    int related{0};
    bool hasCurrent{false};
    bool hasRelated{false};
};

class TreeSession : private TreeObserver<int> {
private:
    BinaryTree<int> tree;
    ArraySequence<TreeFrame> frames;
    ArraySequence<int> visited;
    int argument{0};
    bool recordingFailed{false};

    void Capture(const std::string& action, const std::string& message,
                 const int* current = nullptr, const int* related = nullptr);
    void OnTreeEvent(TreeEvent event, const int* current, const int* related) noexcept override;
    void Traverse(int order);
    void MakeRandom(int seed);
    void PrepareRotation(int choice);

public:
    TreeSession();

    void Execute(const std::string& action, int value = 0);
    const ArraySequence<TreeFrame>& GetFrames() const;
    std::string ToJson() const;
};
