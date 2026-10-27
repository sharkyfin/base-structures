#pragma once

template <class T, class Compare>
void BinaryHeap<T, Compare>::Up(int index) {
    while (index > 0) {
        const int parent = (index - 1) / 2;
        if (!compare(values.Get(parent), values.Get(index))) break;
        Swap(values.GetMutable(parent), values.GetMutable(index));
        index = parent;
    }
}

template <class T, class Compare>
void BinaryHeap<T, Compare>::Down(int index) {
    const int count = GetCount();
    while (index < count / 2) {
        int child = index * 2 + 1;
        if (child + 1 < count && compare(values.Get(child), values.Get(child + 1))) ++child;
        if (!compare(values.Get(index), values.Get(child))) break;
        Swap(values.GetMutable(index), values.GetMutable(child));
        index = child;
    }
}

template <class T, class Compare>
void BinaryHeap<T, Compare>::EraseAt(int index) {
    const int last = GetCount() - 1;
    if (index != last) values.GetMutable(index) = Move(values.GetMutable(last));
    values.RemoveAt(last);
    if (index < GetCount()) {
        if (index > 0 && compare(values.Get((index - 1) / 2), values.Get(index))) Up(index);
        else Down(index);
    }
}

template <class T, class Compare>
BinaryHeap<T, Compare>::BinaryHeap(Compare compare) : compare(Move(compare)) {}

template <class T, class Compare>
BinaryHeap<T, Compare>::BinaryHeap(const ICollectible<T>& source, Compare compare) : values(source), compare(Move(compare)) {
    for (int i = GetCount() / 2; i > 0; --i) Down(i - 1);
}

template <class T, class Compare>
int BinaryHeap<T, Compare>::GetCount() const { return values.GetCount(); }

template <class T, class Compare>
const Compare& BinaryHeap<T, Compare>::GetCompare() const noexcept { return compare; }

template <class T, class Compare>
void BinaryHeap<T, Compare>::Clear() { values.Clear(); }

template <class T, class Compare>
void BinaryHeap<T, Compare>::Add(T value) {
    values.Append(Move(value));
    Up(GetCount() - 1);
}

template <class T, class Compare>
const T& BinaryHeap<T, Compare>::Peek() const { return values.GetFirst(); }

template <class T, class Compare>
T BinaryHeap<T, Compare>::Pop() {
    T result = Peek();
    EraseAt(0);
    return result;
}

template <class T, class Compare>
bool BinaryHeap<T, Compare>::Contains(const T& value) const {
    for (int i = 0; i < GetCount(); ++i) {
        if (values.Get(i) == value) return true;
    }
    return false;
}

template <class T, class Compare>
bool BinaryHeap<T, Compare>::Remove(const T& value) {
    for (int i = 0; i < GetCount(); ++i) {
        if (values.Get(i) == value) { EraseAt(i); return true; }
    }
    return false;
}

template <class T, class Compare>
bool BinaryHeap<T, Compare>::IsValid() const {
    for (int i = 1; i < GetCount(); ++i) {
        if (compare(values.Get((i - 1) / 2), values.Get(i))) return false;
    }
    return true;
}

template <class T, class Compare>
UniquePtr<IEnumerator<T>> BinaryHeap<T, Compare>::GetEnumerator() const { return values.GetEnumerator(); }
