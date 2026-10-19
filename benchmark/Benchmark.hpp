#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>

#include "heaps/BinaryHeap.hpp"
#include "queues/PriorityQueue.hpp"
#include "sets/BinaryTreeSet.hpp"
#include "trees/BinaryTree.hpp"

struct Measurement {
    int completed{0};
    double milliseconds{0};
    bool timedOut{false};
};

template <class Function>
Measurement Measure(int operations, double seconds, Function function);

inline void PrintMeasurement(const char* structure, const char* operation, const char* order, int records, const Measurement& result);

inline void CheckBenchmark(bool condition);

inline void BenchmarkTree(const ArraySequence<int>& input, const char* order, double seconds);

inline void BenchmarkSet(const ArraySequence<int>& input, const char* order, double seconds);

template <class Collection>
void BenchmarkPriority(const ArraySequence<int>& input, const char* order, double seconds,
    Collection collection, const char* name, void (Collection::*insert)(int), int (Collection::*extract)(),
    const char* insertName = "enqueue", const char* extractName = "dequeue");

inline void RunBenchmarks(int maxCount = 1000000, double seconds = 1.0);

#include "Benchmark.tpp"
