#pragma once

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

template <class Collection>
void BenchmarkPriority(const ArraySequence<int>& input, const char* order, double seconds,
    Collection collection, const char* name, void (Collection::*insert)(int), int (Collection::*extract)(),
    const char* insertName, const char* extractName) {
    const int requested = input.GetCount();
    auto result = Measure(requested, seconds, [&](int i) { (collection.*insert)(input.Get(i)); });
    PrintMeasurement(name, insertName, order, requested, result);
    const int count = collection.GetCount();
    CheckBenchmark(collection.IsValid() && count == result.completed);
    int value = 0;
    result = Measure(count, seconds, [&](int) { value = (collection.*extract)(); });
    CheckBenchmark(collection.GetCount() == count - result.completed && collection.IsValid());
    if (result.completed == count && count == requested) CheckBenchmark(value == 0);
    PrintMeasurement(name, extractName, order, count, result);
}

inline void RunBenchmarks(int maxCount, double seconds) {
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
                BenchmarkPriority(input, orders[order], seconds, PriorityQueue<int>(MakeUnique<TreePriorityStorage<int>>()),
                    "QueueAVL", &PriorityQueue<int>::Enqueue, &PriorityQueue<int>::Dequeue);
                BenchmarkPriority(input, orders[order], seconds, PriorityQueue<int>(MakeUnique<HeapPriorityStorage<int>>()),
                    "QueueHeap", &PriorityQueue<int>::Enqueue, &PriorityQueue<int>::Dequeue);
                BenchmarkPriority(input, orders[order], seconds, BinaryHeap<int>(),
                    "Heap", &BinaryHeap<int>::Add, &BinaryHeap<int>::Pop, "insert", "pop");
            } catch (const std::bad_alloc&) {
                std::cout << "N=" << count << ' ' << orders[order] << ": недостаточно памяти, дальнейшие размеры пропущены.\n";
                return;
            }
        }
    }
}
