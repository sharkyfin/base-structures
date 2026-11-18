#pragma once

#include "benchmark/Benchmark.hpp"

struct BenchmarkPoint {
    int operation;
    int order;
    int count;
    double total{0};
    DynamicArray<int> repetitions;

    double Mean() const { return total / repetitions.GetSize(); }
};

struct BenchmarkReport {
    DynamicArray<BenchmarkPoint> points;
    int maxCount{0};
    int repetitions{0};
    int queries{0};
    unsigned int seed{0};
};

BenchmarkReport ReadBenchmarkReport(std::istream& input);
BenchmarkReport ReadBenchmarkSummary(std::istream& input);
void SaveBenchmarkReport(const BenchmarkReport& report, const std::string& directory);
void BuildBenchmarkReport(std::istream& input, const std::string& directory);
void RunBenchmarkReport(int maxCount = 1000000, int repetitions = 7, unsigned int seed = 42,
    int queries = 100000, const std::string& directory = "benchmark/results");
