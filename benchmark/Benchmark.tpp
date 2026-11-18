#pragma once

template <class Function>
Measurement Measure(int operations, Function function) {
    if (operations <= 0) throw std::invalid_argument("Positive operation count required.");
    Measurement result;
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < operations; ++i) {
        // Compiler barrier, not a CPU fence: do not hoist repeated Peek/Find calls.
#if defined(__GNUC__) || defined(__clang__)
        asm volatile("" ::: "memory");
#else
#error "Benchmark barrier requires GCC or Clang."
#endif
        result.checksum += function(i);
    }
    result.milliseconds = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();
    result.completed = operations;
    return result;
}

inline void CheckBenchmark(bool condition) {
    if (!condition) throw std::logic_error("Benchmark result is incorrect.");
}

inline void BenchmarkOutput::Write(const char* structure, const char* operation, const Measurement& result) const {
    if (repetition < 0) return;
    stream << structure << ',' << operation << ',' << order << ',' << count << ','
           << result.completed << ',' << repetition + 1 << ',' << seed << ','
           << std::fixed << std::setprecision(9) << result.milliseconds << ',' << result.checksum << ",ok\n";
    if (!stream) throw std::runtime_error("Cannot write benchmark CSV.");
}

inline ArraySequence<int> BenchmarkSizes(int maxCount) {
    if (maxCount < 1000 || maxCount > 100000000) {
        throw std::invalid_argument("Use 1000..100000000 records.");
    }
    ArraySequence<int> sizes;
    for (int scale = 1000; scale <= maxCount; scale *= 10) {
        for (int digit = 1; digit <= 9 && digit * scale <= maxCount; ++digit) sizes.Append(digit * scale);
    }
    if (sizes.GetLast() != maxCount) sizes.Append(maxCount);
    return sizes;
}

inline ArraySequence<int> BenchmarkInput(int count, int order, unsigned int seed) {
    DynamicArray<int> values;
    values.Reserve(count);
    for (int i = 0; i < count; ++i) values.Append(2 * i);
    if (order == 1) Reverse(values.begin(), values.end());
    if (order == 2) {
        std::mt19937 random(seed);
        Shuffle(values.begin(), values.end(), random);
    }
    return ArraySequence<int>(Move(values));
}

template <class Contains>
void BenchmarkSearch(const BenchmarkOutput& output, const char* name,
    const ArraySequence<int>& queries, Contains contains) {
    auto result = Measure(queries.GetCount(), [&](int i) { return contains(queries.Get(i)); });
    CheckBenchmark(result.checksum == queries.GetCount());
    output.Write(name, "find_hit", result);
    result = Measure(queries.GetCount(), [&](int i) { return contains(queries.Get(i) + 1); });
    CheckBenchmark(result.checksum == 0);
    output.Write(name, "find_miss", result);
}

inline void BenchmarkTree(const BenchmarkOutput& output, const ArraySequence<int>& input,
    const ArraySequence<int>& randomInput, const ArraySequence<int>& queries) {
    const int count = input.GetCount();
    BinaryTree<int> tree;
    auto result = Measure(count, [&](int i) { return tree.Add(input.Get(i)); });
    CheckBenchmark(tree.IsValid() && tree.GetCount() == count && result.checksum == count);
    output.Write("AVL", "insert", result);
    BenchmarkSearch(output, "AVL", queries, [&](int key) { return tree.Find(key) != nullptr; });

    for (Traversal order : {Traversal::LKP, Traversal::BreadthFirst}) {
        UniquePtr<IEnumerator<int>> enumerator;
        int visited = 0;
        // Include enumerator creation and the final MoveNext in the full traversal.
        result = Measure(1, [&](int) {
            enumerator = tree.GetEnumerator(order);
            long long sum = 0;
            while (enumerator->MoveNext()) { sum += enumerator->CurrentRef(); ++visited; }
            return sum;
        });
        CheckBenchmark(visited == count && result.checksum == 1LL * count * (count - 1));
        output.Write("AVL", order == Traversal::LKP ? "traverse_lkp" : "traverse_bfs", result);
    }

    // All deletion orders start from the same tree, built outside the timer.
    tree.Clear();
    for (int i = 0; i < count; ++i) tree.Add(randomInput.Get(i));
    result = Measure(count, [&](int i) { return tree.Remove(input.Get(i)); });
    CheckBenchmark(result.checksum == count && tree.IsEmpty() && tree.IsValid());
    output.Write("AVL", "remove", result);
}

inline void CheckBenchmarkSet(const BinaryTreeSet<int>& set, int first, int count) {
    CheckBenchmark(set.GetCount() == count && set.IsValid());
    auto enumerator = set.GetEnumerator();
    int index = 0;
    while (enumerator->MoveNext()) CheckBenchmark(enumerator->CurrentRef() == 2 * (first + index++));
    CheckBenchmark(index == count);
}

inline void BenchmarkSet(const BenchmarkOutput& output, const ArraySequence<int>& input,
    const ArraySequence<int>& randomInput, const ArraySequence<int>& queries) {
    const int count = input.GetCount();
    BinaryTreeSet<int> set;
    auto result = Measure(count, [&](int i) { return set.Add(input.Get(i)); });
    CheckBenchmark(result.checksum == count);
    CheckBenchmarkSet(set, 0, count);
    output.Write("Set", "insert", result);
    BenchmarkSearch(output, "Set", queries, [&](int key) { return set.Contains(key); });

    const int offset = count - count / 2;
    BinaryTreeSet<int> other;
    for (int i = 0; i < count; ++i) other.Add(input.Get(i) + 2 * offset);
    for (int operation = 0; operation < 3; ++operation) {
        BinaryTreeSet<int> combined;
        result = Measure(1, [&](int) {
            if (operation == 0) combined = set | other;
            else if (operation == 1) combined = set & other;
            else combined = set - other;
            return combined.GetCount();
        });
        const int expected = operation == 0 ? count + offset : operation == 1 ? count / 2 : offset;
        CheckBenchmarkSet(combined, operation == 1 ? offset : 0, expected);
        const char* names[]{"union", "intersection", "difference"};
        output.Write("Set", names[operation], result);
        // combined is destroyed after timing and validation.
    }

    set.Clear();
    for (int i = 0; i < count; ++i) set.Add(randomInput.Get(i));
    result = Measure(count, [&](int i) { return set.Remove(input.Get(i)); });
    CheckBenchmark(result.checksum == count && set.IsEmpty() && set.IsValid());
    output.Write("Set", "remove", result);
}

template <class Collection>
void BenchmarkPriority(const BenchmarkOutput& output, const ArraySequence<int>& input, int queries,
    Collection collection, const char* name, void (Collection::*insert)(int), int (Collection::*extract)(),
    const char* insertName, const char* extractName) {
    const int count = input.GetCount();
    auto result = Measure(count, [&](int i) { (collection.*insert)(input.Get(i)); return 0; });
    CheckBenchmark(collection.IsValid() && collection.GetCount() == count);
    output.Write(name, insertName, result);
    result = Measure(queries, [&](int) { return collection.Peek(); });
    CheckBenchmark(result.checksum == 2LL * (count - 1) * queries);
    output.Write(name, "peek", result);

    DynamicArray<int> extracted;
    extracted.Resize(count);
    result = Measure(count, [&](int i) { return extracted[i] = (collection.*extract)(); });
    CheckBenchmark(collection.IsEmpty() && collection.IsValid() && result.checksum == 1LL * count * (count - 1));
    for (int i = 0; i < count; ++i) CheckBenchmark(extracted[i] == 2 * (count - i - 1));
    output.Write(name, extractName, result);
}

inline void BenchmarkHeapBuild(const BenchmarkOutput& output, const ArraySequence<int>& input) {
    BinaryHeap<int> heap;
    const auto result = Measure(1, [&](int) { heap = BinaryHeap<int>(input); return heap.GetCount(); });
    CheckBenchmark(heap.GetCount() == input.GetCount() && heap.IsValid());
    auto enumerator = heap.GetEnumerator();
    DynamicArray<int> seen;
    seen.Resize(input.GetCount(), 0);
    while (enumerator->MoveNext()) {
        const int value = enumerator->CurrentRef();
        CheckBenchmark(value >= 0 && value % 2 == 0 && value / 2 < seen.GetSize());
        CheckBenchmark(++seen[value / 2] == 1);
    }
    for (int i = 0; i < seen.GetSize(); ++i) CheckBenchmark(seen[i] == 1);
    output.Write("Heap", "build", result);
}

inline void RunBenchmarks(int maxCount, int repetitions, unsigned int seed, std::ostream& output, int queries) {
    const auto sizes = BenchmarkSizes(maxCount);
    if (repetitions < 1 || repetitions > 1000 || queries < 1) {
        throw std::invalid_argument("Use 1..1000 repetitions and a positive query count.");
    }
    output << BenchmarkCsvHeader << '\n';
    const char* orders[]{"Ascending", "Descending", "Random"};
    for (int size = 0; size < sizes.GetCount(); ++size) {
        const int count = sizes.Get(size);
        std::cerr << "N=" << count << " (" << size + 1 << '/' << sizes.GetCount() << ")\n";
        for (int repetition = -1; repetition < repetitions; ++repetition) {
            const unsigned int sampleSeed = seed + 1664525U * static_cast<unsigned int>(count)
                + 1013904223U * static_cast<unsigned int>(repetition + 1);
            const auto randomInput = BenchmarkInput(count, 2, sampleSeed);
            ArraySequence<int> search;
            search.Reserve(queries);
            std::mt19937 random(sampleSeed ^ 0x9e3779b9U);
            std::uniform_int_distribution<int> index(0, count - 1);
            for (int i = 0; i < queries; ++i) search.Append(2 * index(random));
            for (int step = 0; step < 3; ++step) {
                const int order = (step + repetition + 1) % 3;
                const auto input = order == 2 ? randomInput : BenchmarkInput(count, order, sampleSeed);
                const BenchmarkOutput writer{output, orders[order], count, repetition, sampleSeed};
                // Rotate the implementation order between repetitions.
                for (int implementation = 0; implementation < 5; ++implementation) {
                    switch ((implementation + repetition + 1) % 5) {
                        case 0: BenchmarkTree(writer, input, randomInput, search); break;
                        case 1: BenchmarkSet(writer, input, randomInput, search); break;
                        case 2:
                            BenchmarkPriority(writer, input, queries, PriorityQueue<int>(MakeUnique<TreePriorityStorage<int>>()),
                                "QueueAVL", &PriorityQueue<int>::Enqueue, &PriorityQueue<int>::Dequeue, "enqueue", "dequeue");
                            break;
                        case 3:
                            BenchmarkPriority(writer, input, queries, PriorityQueue<int>(MakeUnique<HeapPriorityStorage<int>>()),
                                "QueueHeap", &PriorityQueue<int>::Enqueue, &PriorityQueue<int>::Dequeue, "enqueue", "dequeue");
                            break;
                        case 4:
                            BenchmarkPriority(writer, input, queries, BinaryHeap<int>(),
                                "Heap", &BinaryHeap<int>::Add, &BinaryHeap<int>::Pop, "insert", "pop");
                            BenchmarkHeapBuild(writer, input);
                            break;
                    }
                }
            }
        }
        output.flush();
    }
}
