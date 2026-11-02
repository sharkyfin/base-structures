#pragma once

enum class TreeEvent {
    Compare, Inserted, Duplicate, Found, Missing, Removing,
    DetachedMinimum, Removed, Imbalance, RotatedLeft, RotatedRight
};

template <class T>
class TreeObserver {
public:
    virtual ~TreeObserver() = default;

    // Values are borrowed for this call. Observers must not change the tree.
    virtual void OnTreeEvent(TreeEvent event, const T* current, const T* related) noexcept = 0;
};
