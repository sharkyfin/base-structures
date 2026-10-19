#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>

#include "heaps/BinaryHeap.hpp"
#include "queues/PriorityQueue.hpp"
#include "sets/BinaryTreeSet.hpp"
#include "trees/NaryTree.hpp"

struct Measurement {
    int completed{0};
    double milliseconds{0};
    bool timedOut{false};
};

template <class Function>
Measurement Measure(int operations, double seconds, Function function) {
    Measurement result;
    const auto start = std::chrono::steady_clock::now();
    while (result.completed < operations) {
        function(result.completed);
        ++result.completed;
        if (result.completed % 1024 == 0 || result.completed == operations) {
            const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            if (elapsed >= seconds) { result.timedOut = true; break; }
        }
    }
    result.milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    return result;
}

inline void PrintMeasurement(const char* structure, const char* operation, const char* order, int records, const Measurement& result) {
    std::cout << std::left << std::setw(10) << structure << std::setw(14) << operation << std::setw(12) << order
        << std::right << std::setw(11) << records << std::setw(12) << result.completed
        << std::setw(13) << std::fixed << std::setprecision(3) << result.milliseconds
        << "  " << (result.timedOut ? "limit" : "ok") << '\n';
}

inline void CheckBenchmark(bool condition) {
    if (!condition) throw std::logic_error("Benchmark result is incorrect.");
}

inline void BenchmarkTree(const ArraySequence<int>& input, const char* order, double seconds) {
    BinaryTree<int> tree;
    const int requested = input.GetCount();
    auto result = Measure(requested, seconds, [&](int i) { tree.Add(input.Get(i)); });
    PrintMeasurement("AVL", "insert", order, requested, result);
    const int count = tree.GetCount();
    CheckBenchmark(tree.IsValid() && count == result.completed);
    int found = 0;
    result = Measure(100000, seconds, [&](int i) { found += tree.Find(i % 2 == 0 ? input.Get(i % count) : -1) != nullptr; });
    CheckBenchmark(found == (result.completed + 1) / 2);
    PrintMeasurement("AVL", "find", order, count, result);
    auto enumerator = tree.GetEnumerator();
    int visited = 0;
    result = Measure(count, seconds, [&](int) { if (enumerator->MoveNext()) ++visited; });
    CheckBenchmark(visited == result.completed);
    PrintMeasurement("AVL", "traverse", order, count, result);
    int removed = 0;
    result = Measure(count, seconds, [&](int i) { removed += tree.Remove(input.Get(i)); });
    CheckBenchmark(removed == result.completed && tree.GetCount() == count - removed && tree.IsValid());
    PrintMeasurement("AVL", "remove", order, count, result);
}

inline void BenchmarkSet(const ArraySequence<int>& input, const char* order, double seconds) {
    BinaryTreeSet<int> set;
    const int requested = input.GetCount();
    auto result = Measure(requested, seconds, [&](int i) { set.Add(input.Get(i)); });
    PrintMeasurement("Set", "insert", order, requested, result);
    CheckBenchmark(set.IsValid() && set.GetCount() == result.completed);
    const int count = set.GetCount();
    const auto subset = set.Where([](int value) { return value % 2 == 0; });
    int matched = 0;
    result = Measure(1, seconds, [&](int) { matched = (set & subset).GetCount(); });
    CheckBenchmark(matched == subset.GetCount());
    PrintMeasurement("Set", "intersection", order, count, result);
    result = Measure(1, seconds, [&](int) { matched = (set | subset).GetCount(); });
    CheckBenchmark(matched == count);
    PrintMeasurement("Set", "union", order, count, result);
    result = Measure(1, seconds, [&](int) { matched = (set - subset).GetCount(); });
    CheckBenchmark(matched == count - subset.GetCount());
    PrintMeasurement("Set", "difference", order, count, result);
}

inline void BenchmarkQueue(const ArraySequence<int>& input, const char* order, double seconds,
    UniquePtr<IPriorityStorage<int>> storage, const char* name) {
    PriorityQueue<int> queue(Move(storage));
    const int requested = input.GetCount();
    auto result = Measure(requested, seconds, [&](int i) { queue.Enqueue(input.Get(i)); });
    PrintMeasurement(name, "enqueue", order, requested, result);
    const int count = queue.GetCount();
    CheckBenchmark(queue.IsValid() && count == result.completed);
    int value = 0;
    result = Measure(count, seconds, [&](int) { value = queue.Dequeue(); });
    CheckBenchmark(queue.GetCount() == count - result.completed && queue.IsValid());
    if (result.completed == count && count == requested) CheckBenchmark(value == 0);
    PrintMeasurement(name, "dequeue", order, count, result);
}

inline void BenchmarkHeap(const ArraySequence<int>& input, const char* order, double seconds) {
    BinaryHeap<int> heap;
    const int requested = input.GetCount();
    auto result = Measure(requested, seconds, [&](int i) { heap.Add(input.Get(i)); });
    PrintMeasurement("Heap", "insert", order, requested, result);
    const int count = heap.GetCount();
    CheckBenchmark(heap.IsValid() && count == result.completed);
    int value = 0;
    result = Measure(count, seconds, [&](int) { value = heap.Pop(); });
    CheckBenchmark(heap.GetCount() == count - result.completed && heap.IsValid());
    if (result.completed == count && count == requested) CheckBenchmark(value == 0);
    PrintMeasurement("Heap", "pop", order, count, result);
}

inline void BenchmarkNaryTree(const ArraySequence<int>& input, const char* order, double seconds) {
    NaryTree<int> tree(3);
    const int requested = input.GetCount();
    auto result = Measure(requested, seconds, [&](int i) { tree.Add(input.Get(i)); });
    PrintMeasurement("Nary(3)", "insert", order, requested, result);
    const int count = tree.GetCount();
    CheckBenchmark(count == result.completed);
    bool absent = false;
    result = Measure(1, seconds, [&](int) { absent = tree.Find(-1) == nullptr; });
    CheckBenchmark(absent);
    PrintMeasurement("Nary(3)", "find", order, count, result);
    auto enumerator = tree.GetEnumerator();
    int visited = 0;
    result = Measure(count, seconds, [&](int) { if (enumerator->MoveNext()) ++visited; });
    CheckBenchmark(visited == result.completed);
    PrintMeasurement("Nary(3)", "traverse", order, count, result);
    bool removed = false;
    result = Measure(1, seconds, [&](int) { removed = tree.Remove(input.Get(count - 1)); });
    CheckBenchmark(removed && tree.GetCount() == count - 1);
    PrintMeasurement("Nary(3)", "remove", order, count, result);
}

inline void RunBenchmarks(int maxCount = 1000000, double seconds = 1.0) {
    if (maxCount < 10000 || maxCount > 100000000 || !(seconds > 0 && seconds <= 3600)) {
        throw std::invalid_argument("Use 10000..100000000 records and 0 < seconds <= 3600.");
    }
    std::cout << "Структура Операция      Порядок               N    Операций           мс  Статус\n";
    const int sizes[]{10000, 100000, 1000000, 10000000, 100000000};
    const char* orders[]{"Ascending", "Descending", "Random"};
    std::mt19937 random(42);
    for (int count : sizes) {
        if (count > maxCount) break;
        for (int order = 0; order < 3; ++order) {
            try {
                ArraySequence<int> input;
                input.Reserve(count);
                for (int i = 0; i < count; ++i) input.Append(order == 1 ? count - i - 1 : i);
                if (order == 2) {
                    for (int i = count - 1; i > 0; --i) {
                        std::uniform_int_distribution<int> position(0, i);
                        Swap(input.GetMutable(i), input.GetMutable(position(random)));
                    }
                }
                BenchmarkTree(input, orders[order], seconds);
                BenchmarkSet(input, orders[order], seconds);
                BenchmarkQueue(input, orders[order], seconds, MakeUnique<TreePriorityStorage<int>>(), "QueueAVL");
                BenchmarkQueue(input, orders[order], seconds, MakeUnique<HeapPriorityStorage<int>>(), "QueueHeap");
                BenchmarkHeap(input, orders[order], seconds);
                BenchmarkNaryTree(input, orders[order], seconds);
            } catch (const std::bad_alloc&) {
                std::cout << "N=" << count << ' ' << orders[order] << ": недостаточно памяти, дальнейшие размеры пропущены.\n";
                return;
            }
        }
    }
}
