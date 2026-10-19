#pragma once

template <class T>
PriorityQueue<T>::PriorityQueue(UniquePtr<IPriorityStorage<T>> priorityStorage) : storage(Move(priorityStorage)) {
    if (!storage) throw std::invalid_argument("Priority storage is required.");
}

template <class T>
PriorityQueue<T>::PriorityQueue(const PriorityQueue& other) : storage(other.storage->Clone()) {}

template <class T>
PriorityQueue<T>::PriorityQueue(PriorityQueue&& other) : storage(other.storage->CreateEmpty()) {
    storage.swap(other.storage);
}

template <class T>
PriorityQueue<T>& PriorityQueue<T>::operator=(const PriorityQueue& other) {
    if (this != &other) {
        PriorityQueue copy(other);
        Swap(copy);
    }
    return *this;
}

template <class T>
PriorityQueue<T>& PriorityQueue<T>::operator=(PriorityQueue&& other) {
    if (this != &other) {
        PriorityQueue moved(Move(other));
        Swap(moved);
    }
    return *this;
}

template <class T>
void PriorityQueue<T>::Swap(PriorityQueue& other) noexcept { storage.swap(other.storage); }

template <class T>
int PriorityQueue<T>::GetCount() const { return storage->GetCount(); }

template <class T>
bool PriorityQueue<T>::IsValid() const { return storage->IsValid(); }

template <class T>
void PriorityQueue<T>::Clear() { storage->Clear(); }

template <class T>
void PriorityQueue<T>::Enqueue(T value) { storage->Add(Move(value)); }

template <class T>
const T& PriorityQueue<T>::Peek() const { return storage->Peek(); }

template <class T>
T PriorityQueue<T>::Dequeue() { return storage->Pop(); }

template <class T>
bool PriorityQueue<T>::Contains(const T& value) const {
    auto enumerator = GetEnumerator();
    while (enumerator->MoveNext()) {
        if (enumerator->CurrentRef() == value) return true;
    }
    return false;
}

template <class T>
template <class Transform>
PriorityQueue<T> PriorityQueue<T>::Map(Transform transform) const {
    PriorityQueue result(storage->CreateEmpty());
    auto enumerator = GetEnumerator();
    while (enumerator->MoveNext()) result.Enqueue(transform(enumerator->CurrentRef()));
    return result;
}

template <class T>
template <class Predicate>
PriorityQueue<T> PriorityQueue<T>::Where(Predicate predicate) const {
    PriorityQueue result(storage->CreateEmpty());
    auto enumerator = GetEnumerator();
    while (enumerator->MoveNext()) {
        const T& value = enumerator->CurrentRef();
        if (predicate(value)) result.Enqueue(value);
    }
    return result;
}

template <class T>
template <class Accumulator, class Function>
Accumulator PriorityQueue<T>::Reduce(Accumulator initial, Function function) const {
    auto enumerator = GetEnumerator();
    while (enumerator->MoveNext()) initial = function(enumerator->CurrentRef(), Move(initial));
    return initial;
}

template <class T>
ArraySequence<T> PriorityQueue<T>::GetSubsequence(int first, int last) const {
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

template <class T>
bool PriorityQueue<T>::ContainsSubsequence(const Sequence<T>& pattern) const {
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

template <class T>
PriorityQueue<T> PriorityQueue<T>::Merge(const IEnumerable<T>& other) const {
    PriorityQueue result(*this);
    auto enumerator = other.GetEnumerator();
    while (enumerator->MoveNext()) result.Enqueue(enumerator->CurrentRef());
    return result;
}

template <class T>
template <class Predicate>
Pair<PriorityQueue<T>, PriorityQueue<T>> PriorityQueue<T>::Split(Predicate predicate) const {
    Pair<PriorityQueue, PriorityQueue> result{PriorityQueue(storage->CreateEmpty()), PriorityQueue(storage->CreateEmpty())};
    auto enumerator = GetEnumerator();
    while (enumerator->MoveNext()) {
        const T& value = enumerator->CurrentRef();
        if (predicate(value)) result.key.Enqueue(value);
        else result.value.Enqueue(value);
    }
    return result;
}

template <class T>
template <class Writer>
std::string PriorityQueue<T>::ToString(Writer write) const { return WriteValues(GetEnumerator(), write); }

template <class T>
std::string PriorityQueue<T>::ToString() const { return ToString([](const T& value) { return WriteText(value); }); }

template <class T>
template <class Reader>
PriorityQueue<T> PriorityQueue<T>::FromString(const std::string& text, Reader read,
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

template <class T>
PriorityQueue<T> PriorityQueue<T>::FromString(const std::string& text,
        UniquePtr<IPriorityStorage<T>> priorityStorage) {
    return FromString(text, [](const std::string& value) { return ReadText<T>(value); }, Move(priorityStorage));
}

template <class T>
UniquePtr<IEnumerator<T>> PriorityQueue<T>::GetEnumerator() const { return storage->GetEnumerator(); }
