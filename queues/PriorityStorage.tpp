#pragma once

template <class T, class Compare>
TreePriorityStorage<T, Compare>::TreePriorityStorage(Compare compare) : tree(Move(compare), true) {}

template <class T, class Compare>
int TreePriorityStorage<T, Compare>::GetCount() const { return tree.GetCount(); }

template <class T, class Compare>
void TreePriorityStorage<T, Compare>::Add(T value) { tree.Add(Move(value)); }

template <class T, class Compare>
const T& TreePriorityStorage<T, Compare>::Peek() const { return tree.Maximum(); }

template <class T, class Compare>
T TreePriorityStorage<T, Compare>::Pop() { return tree.PopMaximum(); }

template <class T, class Compare>
void TreePriorityStorage<T, Compare>::Clear() { tree.Clear(); }

template <class T, class Compare>
bool TreePriorityStorage<T, Compare>::IsValid() const { return tree.IsValid(); }

template <class T, class Compare>
UniquePtr<IEnumerator<T>> TreePriorityStorage<T, Compare>::GetEnumerator() const { return tree.GetEnumerator(Traversal::PKL); }

template <class T, class Compare>
UniquePtr<IPriorityStorage<T>> TreePriorityStorage<T, Compare>::Clone() const { return MakeUnique<TreePriorityStorage>(*this); }

template <class T, class Compare>
UniquePtr<IPriorityStorage<T>> TreePriorityStorage<T, Compare>::CreateEmpty() const {
    return MakeUnique<TreePriorityStorage>(tree.GetCompare());
}

template <class T, class Compare>
HeapPriorityStorage<T, Compare>::Enumerator::Enumerator(const BinaryHeap<T, Compare>& heap) {
    values.Reserve(heap.GetCount());
    auto enumerator = heap.GetEnumerator();
    while (enumerator->MoveNext()) values.Append(&enumerator->CurrentRef());
    values.Sort([&](const T* first, const T* second) { return heap.GetCompare()(*second, *first); });
}

template <class T, class Compare>
bool HeapPriorityStorage<T, Compare>::Enumerator::MoveNext() {
    if (position < values.GetSize()) ++position;
    return position < values.GetSize();
}

template <class T, class Compare>
const T& HeapPriorityStorage<T, Compare>::Enumerator::CurrentRef() const {
    if (position < 0 || position >= values.GetSize()) throw std::logic_error("Enumerator has no current element.");
    return *values.Get(position);
}

template <class T, class Compare>
HeapPriorityStorage<T, Compare>::HeapPriorityStorage(Compare compare) : heap(Move(compare)) {}

template <class T, class Compare>
int HeapPriorityStorage<T, Compare>::GetCount() const { return heap.GetCount(); }

template <class T, class Compare>
void HeapPriorityStorage<T, Compare>::Add(T value) { heap.Add(Move(value)); }

template <class T, class Compare>
const T& HeapPriorityStorage<T, Compare>::Peek() const { return heap.Peek(); }

template <class T, class Compare>
T HeapPriorityStorage<T, Compare>::Pop() { return heap.Pop(); }

template <class T, class Compare>
void HeapPriorityStorage<T, Compare>::Clear() { heap.Clear(); }

template <class T, class Compare>
bool HeapPriorityStorage<T, Compare>::IsValid() const { return heap.IsValid(); }

template <class T, class Compare>
UniquePtr<IEnumerator<T>> HeapPriorityStorage<T, Compare>::GetEnumerator() const { return MakeUnique<Enumerator>(heap); }

template <class T, class Compare>
UniquePtr<IPriorityStorage<T>> HeapPriorityStorage<T, Compare>::Clone() const { return MakeUnique<HeapPriorityStorage>(*this); }

template <class T, class Compare>
UniquePtr<IPriorityStorage<T>> HeapPriorityStorage<T, Compare>::CreateEmpty() const {
    return MakeUnique<HeapPriorityStorage>(heap.GetCompare());
}
