#include "benchmark/Report.hpp"

#include "pbPlots.hpp"
#include "supportLib.hpp"

namespace {
struct Operation { const char* structure; const char* name; };
const Operation operations[]{
    {"AVL", "insert"}, {"AVL", "find_hit"}, {"AVL", "find_miss"}, {"AVL", "remove"},
    {"AVL", "traverse_lkp"}, {"AVL", "traverse_bfs"},
    {"Set", "insert"}, {"Set", "find_hit"}, {"Set", "find_miss"}, {"Set", "remove"},
    {"Set", "union"}, {"Set", "intersection"}, {"Set", "difference"},
    {"QueueAVL", "enqueue"}, {"QueueAVL", "dequeue"}, {"QueueAVL", "peek"},
    {"QueueHeap", "enqueue"}, {"QueueHeap", "dequeue"}, {"QueueHeap", "peek"},
    {"Heap", "insert"}, {"Heap", "pop"}, {"Heap", "peek"}, {"Heap", "build"}
};
const char* orders[]{"Ascending", "Descending", "Random"};
int FindOperation(const std::string& structure, const std::string& name) {
    for (int i = 0; i < 23; ++i) {
        if (structure == operations[i].structure && name == operations[i].name) return i;
    }
    return -1;
}

bool IsQuery(int operation) {
    const std::string name = operations[operation].name;
    return name == "find_hit" || name == "find_miss" || name == "peek";
}

int ExpectedOperations(int operation, int count, int queries) {
    const std::string name = operations[operation].name;
    if (IsQuery(operation)) return queries;
    if (name == "insert" || name == "remove" || name == "enqueue" || name == "dequeue" || name == "pop") return count;
    return 1;
}

void Require(bool condition) {
    if (!condition) throw std::invalid_argument("Incomplete or invalid benchmark CSV.");
}

void ValidateReport(const BenchmarkReport& report) {
    const auto sizes = BenchmarkSizes(report.maxCount);
    Require(report.points.GetSize() == 23 * 3 * sizes.GetCount());
    for (int p = 0; p < report.points.GetSize(); ++p) {
        const auto& point = report.points.Get(p);
        bool sizeExists = false;
        for (int i = 0; i < sizes.GetCount(); ++i) if (point.count == sizes.Get(i)) sizeExists = true;
        Require(sizeExists && point.repetitions.GetSize() == report.repetitions);
    }
}

// pbPlots owns its temporary arrays; release them after every exported image/file.
struct PlotMemory { ~PlotMemory() { FreeAllocations(); } };

auto* PlotText(const std::string& text) {
    auto* result = CreateStringReferenceLengthValue(text.length(), L' ')->string;
    for (int i = 0; i < static_cast<int>(text.length()); ++i) (*result)[i] = text[i];
    return result;
}

struct Curve {
    int operation{-1};
    const char* label{""};

    Curve() = default;
    Curve(const char* structure, const char* name, const char* label)
        : operation(FindOperation(structure, name)), label(label) { Require(operation >= 0); }
};

void Plot(const BenchmarkReport& report, const std::string& filename, const std::string& title,
    const Curve* curves, int curveCount) {
    PlotMemory memory;
    const int panels = curveCount == 1 ? 1 : 3;
    const int width = 640, height = 480;
    auto* image = CreateImage(width * panels, height, GetWhite());
    RGBA* colors[]{CreateRGBColor(0.12, 0.40, 0.70), CreateRGBColor(0.93, 0.40, 0.08), CreateRGBColor(0.15, 0.58, 0.22)};
    double maximum = 0;
    for (int p = 0; p < report.points.GetSize(); ++p) {
        const auto& point = report.points.Get(p);
        for (int curve = 0; curve < curveCount; ++curve) {
            if (point.operation == curves[curve].operation && point.Mean() > maximum) maximum = point.Mean();
        }
    }
    const auto sizes = BenchmarkSizes(report.maxCount);
    for (int panel = 0; panel < panels; ++panel) {
        auto* settings = GetDefaultScatterPlotSettings();
        settings->width = width;
        settings->height = height;
        settings->autoBoundaries = false;
        settings->xMin = 0;
        settings->xMax = report.maxCount * 1.03;
        settings->yMin = 0;
        settings->yMax = maximum > 0 ? maximum * 1.10 : 1;
        settings->autoPadding = false;
        settings->xPadding = 85;
        settings->yPadding = 100;
        settings->gridColor = CreateRGBColor(0.86, 0.86, 0.86);
        settings->title = PlotText(title + (panels == 1 ? "" : std::string(" / ") + orders[panel]));
        settings->xLabel = PlotText("Number of elements N");
        settings->yLabel = PlotText(IsQuery(curves[0].operation) ? "Time per query (ns)" : "Total time (ms)");
        const int lines = panels == 1 ? 3 : curveCount;
        for (int line = 0; line < lines; ++line) {
            const int operation = curves[panels == 1 ? 0 : line].operation;
            const int order = panels == 1 ? line : panel;
            auto* series = GetDefaultScatterPlotSeriesSettings();
            series->linearInterpolation = sizes.GetCount() > 1;
            series->pointType = toVector(L"dots");
            series->xs = aCreateNumberArray(sizes.GetCount(), 0);
            series->ys = aCreateNumberArray(sizes.GetCount(), 0);
            series->lineThickness = 2;
            series->color = colors[line];
            for (int i = 0; i < sizes.GetCount(); ++i) {
                for (int p = 0; p < report.points.GetSize(); ++p) {
                    const auto& point = report.points.Get(p);
                    if (point.operation == operation && point.order == order && point.count == sizes.Get(i)) {
                        (*series->xs)[i] = point.count;
                        (*series->ys)[i] = point.Mean();
                    }
                }
            }
            settings->scatterPlotSeries->push_back(series);
        }
        auto* canvas = CreateRGBABitmapImageReference();
        auto* error = CreateStringReferenceLengthValue(0, L' ');
        if (!DrawScatterPlotFromSettings(canvas, settings, error)) {
            std::string message = "Cannot draw benchmark plot: ";
            for (wchar_t character : *error->string) message += static_cast<char>(character);
            throw std::runtime_error(message);
        }
        for (int line = 0; line < lines; ++line) {
            const double y = 48 + line * 15;
            DrawLine(canvas->image, 90, y + 5, 118, y + 5, 2, colors[line]);
            DrawText(canvas->image, 128, y, PlotText(panels == 1 ? orders[line] : curves[line].label), colors[line]);
            auto* series = (*settings->scatterPlotSeries)[line];
            for (int i = 0; i < sizes.GetCount(); ++i) {
                DrawFilledCircle(canvas->image, MapXCoordinateBasedOnSettings((*series->xs)[i], settings),
                    MapYCoordinateBasedOnSettings((*series->ys)[i], settings), 3, colors[line]);
            }
        }
        DrawText(canvas->image, 85, height - 35, PlotText("Arithmetic mean of " + std::to_string(report.repetitions) + " runs; warmup excluded"), GetBlack());
        DrawImageOnImage(image, canvas->image, panel * width, 0);
    }
    WriteToFile(ConvertToPNG(image), filename);
}
} // namespace

BenchmarkReport ReadBenchmarkReport(std::istream& input) {
    BenchmarkReport report;
    std::string line;
    Require(static_cast<bool>(std::getline(input, line)) && line == BenchmarkCsvHeader);
    while (std::getline(input, line)) {
        ArraySequence<std::string> fields;
        std::istringstream row(line);
        std::string field;
        while (std::getline(row, field, ',')) fields.Append(field);
        Require(fields.GetCount() == 10 && fields.Get(9) == "ok");
        const int operation = FindOperation(fields.Get(0), fields.Get(1));
        int order = -1;
        for (int i = 0; i < 3; ++i) if (fields.Get(2) == orders[i]) order = i;
        const int count = ReadText<int>(fields.Get(3));
        const int completed = ReadText<int>(fields.Get(4));
        const int repetition = ReadText<int>(fields.Get(5));
        const unsigned int sampleSeed = ReadText<unsigned int>(fields.Get(6));
        const double elapsed = ReadText<double>(fields.Get(7));
        ReadText<long long>(fields.Get(8));
        Require(operation >= 0 && order >= 0 && count >= 1000 && count <= 100000000
            && repetition >= 1 && repetition <= 1000 && elapsed >= 0 && elapsed < 1e12 && completed > 0);
        const unsigned int seed = sampleSeed - 1664525U * static_cast<unsigned int>(count)
            - 1013904223U * static_cast<unsigned int>(repetition);
        if (report.points.IsEmpty()) report.seed = seed;
        Require(report.seed == seed);
        if (IsQuery(operation) && report.queries == 0) report.queries = completed;
        Require(completed == ExpectedOperations(operation, count, report.queries));
        BenchmarkPoint* point = nullptr;
        for (auto& candidate : report.points) {
            if (candidate.operation == operation && candidate.order == order && candidate.count == count) point = &candidate;
        }
        if (!point) {
            report.points.Append(BenchmarkPoint{operation, order, count, 0, {}});
            point = &report.points.Back();
        }
        for (int previous : point->repetitions) Require(previous != repetition);
        point->repetitions.Append(repetition);
        point->total += IsQuery(operation) ? elapsed * 1000000 / completed : elapsed;
        if (count > report.maxCount) report.maxCount = count;
        if (repetition > report.repetitions) report.repetitions = repetition;
    }
    Require(input.eof() && !input.bad());
    ValidateReport(report);
    return report;
}

BenchmarkReport ReadBenchmarkSummary(std::istream& input) {
    BenchmarkReport report;
    std::string line;
    Require(static_cast<bool>(std::getline(input, line)) && line == "structure,operation,order,n,samples,unit,mean");
    while (std::getline(input, line)) {
        ArraySequence<std::string> fields;
        std::istringstream row(line);
        std::string field;
        while (std::getline(row, field, ',')) fields.Append(field);
        Require(fields.GetCount() == 7);
        const int operation = FindOperation(fields.Get(0), fields.Get(1));
        int order = -1;
        for (int i = 0; i < 3; ++i) if (fields.Get(2) == orders[i]) order = i;
        const int count = ReadText<int>(fields.Get(3));
        const int samples = ReadText<int>(fields.Get(4));
        const double mean = ReadText<double>(fields.Get(6));
        Require(operation >= 0 && order >= 0 && count >= 1000 && count <= 100000000
            && samples >= 1 && samples <= 1000 && mean >= 0 && mean < 1e18);
        Require(fields.Get(5) == (IsQuery(operation) ? "ns/query" : "ms"));
        for (const auto& point : report.points) {
            Require(point.operation != operation || point.order != order || point.count != count);
        }
        BenchmarkPoint point{operation, order, count, mean * samples, {}};
        for (int repetition = 1; repetition <= samples; ++repetition) point.repetitions.Append(repetition);
        report.points.Append(Move(point));
        if (count > report.maxCount) report.maxCount = count;
        if (samples > report.repetitions) report.repetitions = samples;
    }
    Require(input.eof() && !input.bad());
    ValidateReport(report);
    return report;
}

static void SaveBenchmarkText(const std::string& filename, const std::string& text) {
    PlotMemory memory;
    auto* bytes = aCreateNumberArray(text.length(), 0);
    for (int i = 0; i < static_cast<int>(text.length()); ++i) (*bytes)[i] = static_cast<unsigned char>(text[i]);
    WriteToFile(bytes, filename);
}

void SaveBenchmarkReport(const BenchmarkReport& report, const std::string& directory) {
    std::ostringstream summary, html;
    summary << "structure,operation,order,n,samples,unit,mean\n" << std::setprecision(12);
    for (int p = 0; p < report.points.GetSize(); ++p) {
        const auto& point = report.points.Get(p);
        const auto& operation = operations[point.operation];
        summary << operation.structure << ',' << operation.name << ',' << orders[point.order] << ',' << point.count
                << ',' << point.repetitions.GetSize() << ',' << (IsQuery(point.operation) ? "ns/query" : "ms") << ',' << point.Mean() << '\n';
    }
    html << "<!doctype html><html lang='ru'><meta charset='utf-8'><title>Бенчмарки структур</title>"
         << "<style>body{font:16px system-ui;max-width:1500px;margin:32px auto;padding:0 20px}img{width:100%}figure{margin:32px 0}</style>"
         << "<h1>Бенчмарки структур данных</h1><p>Среднее арифметическое " << report.repetitions
         << " замеров. Прогрев исключён. Линии без заливки и сглаживания.</p>"
         << "<p><a href='summary.csv'>Средние значения</a></p>";
    const auto draw = [&](const char* file, const char* title, const char* description, Curve first,
        Curve second = {}, Curve third = {}) {
        const Curve curves[]{first, second, third};
        Plot(report, directory + '/' + file + ".png", title, curves, third.operation >= 0 ? 3 : second.operation >= 0 ? 2 : 1);
        html << "<figure><h2>" << description << "</h2><a href='" << file << ".png'><img src='" << file
             << ".png' alt='" << title << "'></a></figure>";
    };
    draw("avl_insert", "AVL: N insertions", "AVL: заполнение", {"AVL", "insert", "Insert"});
    draw("avl_find", "AVL: lookup", "AVL: успешный и неуспешный поиск", {"AVL", "find_hit", "Found"}, {"AVL", "find_miss", "Missing"});
    draw("avl_remove", "AVL: N removals", "AVL: удаление из одинакового начального дерева", {"AVL", "remove", "Remove"});
    draw("avl_traverse", "AVL: full traversal", "AVL: полный обход", {"AVL", "traverse_lkp", "In-order (LKP)"}, {"AVL", "traverse_bfs", "Breadth-first"});
    draw("set_insert", "Set: N insertions", "Множество: заполнение", {"Set", "insert", "Insert"});
    draw("set_find", "Set: lookup", "Множество: успешный и неуспешный поиск", {"Set", "find_hit", "Found"}, {"Set", "find_miss", "Missing"});
    draw("set_remove", "Set: N removals", "Множество: удаление из одинакового начального дерева", {"Set", "remove", "Remove"});
    draw("set_algebra", "Sets: N each, N/2 shared", "Множества: объединение, пересечение, разность", {"Set", "union", "Union"}, {"Set", "intersection", "Intersection"}, {"Set", "difference", "Difference"});
    draw("queue_enqueue", "Queue: N insertions", "Очередь: добавление", {"QueueAVL", "enqueue", "AVL queue"}, {"QueueHeap", "enqueue", "Heap queue"});
    draw("queue_dequeue", "Queue: N extractions", "Очередь: извлечение", {"QueueAVL", "dequeue", "AVL queue"}, {"QueueHeap", "dequeue", "Heap queue"});
    draw("queue_peek", "Priority: peek", "Очередь и куча: просмотр вершины", {"QueueAVL", "peek", "AVL queue"}, {"QueueHeap", "peek", "Heap queue"}, {"Heap", "peek", "Heap directly"});
    draw("heap_build", "Heap: construction", "Куча: N вставок и построение из коллекции", {"Heap", "insert", "N calls to Add"}, {"Heap", "build", "From collection"});
    draw("heap_pop", "Heap: N extractions", "Куча: извлечение", {"Heap", "pop", "Pop"});
    html << "</html>\n";
    SaveBenchmarkText(directory + "/summary.csv", summary.str());
    SaveBenchmarkText(directory + "/index.html", html.str());
    std::cerr << "PNG report: " << directory << "/index.html\n";
}

void BuildBenchmarkReport(std::istream& input, const std::string& directory) {
    SaveBenchmarkReport(ReadBenchmarkSummary(input), directory);
}

void RunBenchmarkReport(int maxCount, int repetitions, unsigned int seed, int queries, const std::string& directory) {
    std::ostringstream measurements;
    RunBenchmarks(maxCount, repetitions, seed, measurements, queries);
    std::istringstream input(measurements.str());
    SaveBenchmarkReport(ReadBenchmarkReport(input), directory);
}
