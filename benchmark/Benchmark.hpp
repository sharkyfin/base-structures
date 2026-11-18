#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>

#include "heaps/BinaryHeap.hpp"
#include "queues/PriorityQueue.hpp"
#include "sets/BinaryTreeSet.hpp"
#include "trees/BinaryTree.hpp"

inline constexpr char BenchmarkCsvHeader[] = "structure,operation,order,n,operations,repetition,seed,milliseconds,checksum,status";

struct Measurement {
    int completed{0};
    double milliseconds{0};
    long long checksum{0};
};

template <class Function>
Measurement Measure(int operations, Function function);

struct BenchmarkOutput {
    std::ostream& stream;
    const char* order;
    int count;
    int repetition; // -1 is the warmup, which is not written to CSV.
    unsigned int seed;

    void Write(const char* structure, const char* operation, const Measurement& result) const;
};

inline ArraySequence<int> BenchmarkSizes(int maxCount);
inline ArraySequence<int> BenchmarkInput(int count, int order, unsigned int seed);
inline void RunBenchmarks(int maxCount = 1000000, int repetitions = 7, unsigned int seed = 42,
    std::ostream& output = std::cout, int queries = 100000);

#include "Benchmark.tpp"
