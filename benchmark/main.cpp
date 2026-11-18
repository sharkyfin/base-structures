#include "benchmark/Report.hpp"

int main(int argc, char** argv) {
    try {
        if (argc >= 2 && std::string(argv[1]) == "--plot") {
            if (argc > 3) throw std::invalid_argument("Usage: structures_benchmark --plot [existing_directory] < summary.csv");
            BuildBenchmarkReport(std::cin, argc == 3 ? argv[2] : "benchmark/results");
            return 0;
        }
        if (argc > 6) throw std::invalid_argument("Usage: structures_benchmark [max_N [repetitions [seed [queries [existing_directory]]]]]");
        RunBenchmarkReport(argc > 1 ? ReadText<int>(argv[1]) : 1000000,
            argc > 2 ? ReadText<int>(argv[2]) : 7,
            argc > 3 ? ReadText<unsigned int>(argv[3]) : 42,
            argc > 4 ? ReadText<int>(argv[4]) : 100000,
            argc > 5 ? argv[5] : "benchmark/results");
    } catch (const std::exception& error) {
        std::cerr << "Benchmark failed: " << error.what() << '\n';
        return 1;
    }
}
