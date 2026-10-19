#include <gtest/gtest.h>

#include "core/IEnumerable.hpp"
#include "heaps/BinaryHeap.hpp"
#include "queues/PriorityQueue.hpp"
#include "sets/BinaryTreeSet.hpp"
#include "benchmark/Benchmark.hpp"
#include "domain/Values.hpp"
#include "trees/NaryTree.hpp"

template <class T>
std::string Elements(const IEnumerable<T>& source) {
    std::string result;
    auto enumerator = source.GetEnumerator();
    while (enumerator->MoveNext()) result += WriteText(enumerator->CurrentRef()) + " ";
    return result;
}

TEST(Tree, BalanceCopyAndMove) {
    const int cases[][3]{{3, 2, 1}, {1, 2, 3}, {3, 1, 2}, {1, 3, 2}};
    for (const auto& values : cases) {
        BinaryTree<int> tree;
        for (int value : values) tree.Add(value);
        EXPECT_TRUE(tree.IsValid());
        EXPECT_EQ(tree.GetHeight(), 2);
        EXPECT_EQ(tree.GetByPath(""), 2);
    }
    BinaryTree<int> tree;
    for (int i = 0; i < 1000; ++i) tree.Add(i);
    EXPECT_FALSE(tree.Add(1));
    auto copy = tree;
    for (int i = 0; i < 1000; i += 2) EXPECT_TRUE(tree.Remove(i));
    EXPECT_TRUE(tree.IsValid());
    EXPECT_EQ(tree.GetCount(), 500);
    EXPECT_EQ(copy.GetCount(), 1000);
    BinaryTree<int> moved(Move(copy));
    EXPECT_TRUE(copy.IsEmpty());
    EXPECT_TRUE(moved.IsValid());
    EXPECT_EQ(moved.Find(-1), nullptr);
    EXPECT_FALSE(moved.Remove(-1));
    auto reverse = [](int a, int b) { return a > b; };
    BinaryTree<int, bool (*)(int, int)> reversed(+reverse);
    for (int i = 0; i < 20; ++i) reversed.Add(i);
    EXPECT_EQ(*reversed.Find(7), 7);
    EXPECT_TRUE(reversed.IsValid());
}

TEST(Tree, TraversalsAndReferences) {
    BinaryTree<int> tree;
    const int values[]{4, 2, 6, 1, 3, 5, 7};
    for (int value : values) tree.Add(value);
    const char* expected[]{"4 2 1 3 6 5 7 ", "4 6 7 5 2 3 1 ", "1 3 2 5 7 6 4 ",
        "1 2 3 4 5 6 7 ", "7 5 6 3 1 2 4 ", "7 6 5 4 3 2 1 ", "4 2 6 1 3 5 7 "};
    for (int i = 0; i < 7; ++i) {
        const Traversal order = static_cast<Traversal>(i);
        auto enumerator = tree.GetEnumerator(order);
        EXPECT_THROW(enumerator->CurrentRef(), std::logic_error);
        std::string actual;
        while (enumerator->MoveNext()) {
            actual += WriteText(enumerator->CurrentRef()) + " ";
            EXPECT_EQ(&enumerator->CurrentRef(), tree.Find(enumerator->CurrentRef()));
        }
        EXPECT_EQ(actual, expected[i]);
        EXPECT_THROW(enumerator->CurrentRef(), std::logic_error);
    }
    auto old = tree.GetEnumerator();
    auto copy = tree;
    tree.Add(8);
    EXPECT_THROW(old->MoveNext(), std::logic_error);
    auto early = tree.GetEnumerator();
    ASSERT_TRUE(early->MoveNext());
    EXPECT_EQ(early->CurrentRef(), 1);
    tree.Clear();
    EXPECT_EQ(Elements(copy), "1 2 3 4 5 6 7 ");
    EXPECT_THROW(early->CurrentRef(), std::logic_error);
    EXPECT_FALSE(tree.GetEnumerator()->MoveNext());
}

TEST(Tree, FunctionsSubtreesAndPaths) {
    BinaryTree<int> tree;
    for (int value = 1; value <= 3; ++value) tree.Add(value);
    EXPECT_EQ(tree.Reduce(4, [](int value, int acc) { return 2 * value + 3 * acc; }), 144);
    EXPECT_EQ(tree.Map([](int) { return 0; }).GetCount(), 1);
    EXPECT_EQ(Elements(tree.Where([](int value) { return value % 2 != 0; })), "1 3 ");
    EXPECT_TRUE(tree.Where([](int) { return false; }).IsEmpty());
    auto subtree = tree.ExtractSubtree(1);
    EXPECT_TRUE(tree.ContainsSubtree(subtree));
    EXPECT_TRUE(tree.ContainsSubtree(BinaryTree<int>()));
    EXPECT_EQ(tree.GetByPath("L"), 1);
    EXPECT_EQ(tree.GetByPath(2, "R"), 3);
    EXPECT_THROW(tree.GetByPath("LL"), std::out_of_range);
    EXPECT_THROW(tree.GetByPath("X"), std::invalid_argument);
    EXPECT_THROW(tree.ExtractSubtree(10), std::out_of_range);
    subtree.Add(0);
    EXPECT_FALSE(tree.ContainsSubtree(subtree));
    const IEnumerable<int>& source = subtree;
    EXPECT_EQ(tree.Merge(source).GetCount(), 4);
}

TEST(Tree, SpaceSeparatedText) {
    const auto tree = BinaryTree<int>::FromString("6 2 4 1 5 3 3");
    EXPECT_EQ(tree.ToString(), "1 2 3 4 5 6");
    EXPECT_TRUE(tree.IsValid());
    const auto copy = BinaryTree<int>::FromString(tree.ToString());
    EXPECT_EQ(Elements(copy), Elements(tree));
    EXPECT_TRUE(copy.IsValid());
    EXPECT_TRUE(BinaryTree<int>::FromString("").IsEmpty());
    EXPECT_TRUE(BinaryTree<int>::FromString(" \t\n ").IsEmpty());
    EXPECT_EQ(BinaryTree<int>().ToString(), "");
    EXPECT_THROW(BinaryTree<int>::FromString("garbage"), std::invalid_argument);
    EXPECT_THROW(BinaryTree<int>::FromString("1 2bad"), std::invalid_argument);
    EXPECT_THROW(BinaryTree<std::string>::FromString("\"unfinished"), std::invalid_argument);
    BinaryTree<std::string> strings;
    strings.Add(std::string("hello \"world\" \\"));
    strings.Add(std::string());
    strings.Add(std::string("plain"));
    EXPECT_EQ(BinaryTree<std::string>::FromString(strings.ToString()).ToString(), strings.ToString());
    BinaryTree<double> decimals;
    decimals.Add(1.2345678901234567);
    EXPECT_DOUBLE_EQ(*BinaryTree<double>::FromString(decimals.ToString()).Find(1.2345678901234567), 1.2345678901234567);
}

TEST(NaryTree, TraversalsAndPaths) {
    EXPECT_THROW(NaryTree<int>(0), std::invalid_argument);
    EXPECT_THROW(NaryTree<int>(-1), std::invalid_argument);
    NaryTree<int> tree(3);
    for (int i = 1; i <= 10; ++i) tree.Add(i);
    const char* expected[]{"1 2 5 6 7 3 8 9 10 4 ", "1 4 3 10 9 8 2 7 6 5 ",
        "5 6 7 2 8 9 10 3 4 1 ", "4 10 9 8 3 7 6 5 2 1 ", "1 2 3 4 5 6 7 8 9 10 "};
    for (int i = 0; i < 5; ++i) {
        auto enumerator = tree.GetEnumerator(static_cast<NaryTraversal>(i));
        EXPECT_THROW(enumerator->CurrentRef(), std::logic_error);
        std::string actual;
        while (enumerator->MoveNext()) {
            actual += WriteText(enumerator->CurrentRef()) + " ";
            EXPECT_EQ(&enumerator->CurrentRef(), tree.Find(enumerator->CurrentRef()));
        }
        EXPECT_EQ(actual, expected[i]);
        EXPECT_THROW(enumerator->CurrentRef(), std::logic_error);
    }
    EXPECT_THROW(tree.GetEnumerator(static_cast<NaryTraversal>(5)), std::invalid_argument);
    ArraySequence<int> path;
    EXPECT_EQ(tree.GetByPath(path), 1);
    EXPECT_THROW(tree.AddChild(path, 11), std::invalid_argument);
    path.Append(1); path.Append(2);
    EXPECT_EQ(tree.GetByPath(path), 10);
    path.RemoveAt(0);
    EXPECT_EQ(tree.GetByPath(3, path), 10);
    EXPECT_THROW(tree.GetByPath(99, path), std::out_of_range);
    path.GetMutable(0) = -1;
    EXPECT_THROW(tree.GetByPath(path), std::out_of_range);
    path.GetMutable(0) = 3;
    EXPECT_THROW(tree.GetByPath(path), std::out_of_range);
    NaryTree<int> uneven(2);
    uneven.Add(1);
    path.Clear(); uneven.AddChild(path, 2);
    path.Append(0); uneven.AddChild(path, 3);
    for (int i = 4; i <= 6; ++i) uneven.Add(i);
    path.GetMutable(0) = 1; path.Append(0);
    EXPECT_EQ(uneven.GetByPath(path), 6);
    NaryTree<int> wide(12);
    for (int i = 0; i <= 12; ++i) wide.Add(i);
    path.Clear(); path.Append(11);
    EXPECT_EQ(wide.GetByPath(path), 12);
}

TEST(NaryTree, FunctionsSubtreesAndOwnership) {
    auto tree = NaryTree<int>::FromString("1 2 3 4 5 6 7 8 9 10", 3);
    auto copy = tree;
    auto subtree = tree.ExtractSubtree(2);
    EXPECT_EQ(subtree.GetCount(), 4);
    EXPECT_TRUE(tree.ContainsSubtree(subtree));
    EXPECT_TRUE(tree.ContainsSubtree(NaryTree<int>(3)));
    subtree.Add(11);
    EXPECT_FALSE(tree.ContainsSubtree(subtree));
    EXPECT_THROW(tree.ExtractSubtree(99), std::out_of_range);
    const auto mapped = tree.Map([](int value) { return -value; });
    EXPECT_EQ(Elements(mapped), "-1 -2 -5 -6 -7 -3 -8 -9 -10 -4 ");
    EXPECT_EQ(tree.Map([](int) { return 0; }).GetCount(), 10);
    EXPECT_EQ(tree.Where([](int value) { return value % 2 == 0; }).ToString(), "2 4 6 8 10");
    EXPECT_EQ(tree.Reduce(0, [](int value, int sum) { return value + sum; }), 55);
    EXPECT_THROW(tree.Map([](int value) { if (value == 5) throw std::runtime_error("stop"); return value; }), std::runtime_error);
    EXPECT_EQ(tree.GetCount(), 10);
    const IEnumerable<int>& source = subtree;
    EXPECT_EQ(tree.Merge(source).GetCount(), 15);
    auto enumerator = tree.GetEnumerator();
    ASSERT_TRUE(enumerator->MoveNext());
    EXPECT_FALSE(tree.Remove(99));
    EXPECT_TRUE(tree.Remove(2));
    EXPECT_EQ(tree.GetCount(), 6);
    EXPECT_EQ(tree.Find(5), nullptr);
    EXPECT_THROW(enumerator->CurrentRef(), std::logic_error);
    EXPECT_EQ(copy.GetCount(), 10);
    ArraySequence<int> path;
    path.Append(0);
    EXPECT_EQ(tree.GetByPath(path), 3);
    tree.Add(11);
    EXPECT_EQ(tree.GetCount(), 7);
    auto old = copy.GetEnumerator();
    NaryTree<int> moved(Move(copy));
    EXPECT_TRUE(copy.IsEmpty());
    EXPECT_THROW(old->MoveNext(), std::logic_error);
    tree = moved;
    EXPECT_EQ(tree.ToString(), moved.ToString());
    tree = Move(moved);
    EXPECT_TRUE(moved.IsEmpty());
    tree.RemoveByPath(path);
    EXPECT_EQ(tree.GetCount(), 6);
    tree.Clear();
    EXPECT_FALSE(tree.GetEnumerator()->MoveNext());
    EXPECT_THROW(tree.RemoveByPath(path), std::out_of_range);
}

TEST(NaryTree, TextAndDeepTrees) {
    NaryTree<std::string> strings(2);
    strings.Add("hello \"world\" \\"); strings.Add(""); strings.Add("same"); strings.Add("same");
    EXPECT_EQ(NaryTree<std::string>::FromString(strings.ToString(), 2).ToString(), strings.ToString());
    EXPECT_TRUE(NaryTree<int>::FromString(" \t\n", 1).IsEmpty());
    EXPECT_THROW(NaryTree<int>::FromString("1 bad", 2), std::invalid_argument);
    EXPECT_THROW(NaryTree<std::string>::FromString("\"unfinished", 2), std::invalid_argument);
    NaryTree<int> chain(1);
    for (int i = 0; i < 20000; ++i) chain.Add(i);
    auto copy = chain;
    const auto mapped = copy.Map([](int value) { return value + 1; });
    EXPECT_EQ(*mapped.Find(20000), 20000);
    chain.Clear();
    EXPECT_EQ(copy.GetCount(), 20000);
    EXPECT_TRUE(copy.Remove(0));
    EXPECT_TRUE(copy.IsEmpty());
}

TEST(Set, AlgebraFunctionsAndText) {
    BinaryTreeSet<int> first;
    BinaryTreeSet<int> second;
    for (int i = 1; i <= 3; ++i) { first.Add(i); second.Add(i + 2); }
    EXPECT_EQ(Elements(first | second), "1 2 3 4 5 ");
    EXPECT_EQ(Elements(*first.Intersection(second)), "3 ");
    EXPECT_EQ(Elements(first - second), "1 2 ");
    EXPECT_TRUE((first & second) <= first);
    EXPECT_TRUE(first == BinaryTreeSet<int>::FromString(first.ToString()));
    EXPECT_TRUE(BinaryTreeSet<int>() <= first);
    EXPECT_TRUE((first - first).IsEmpty());
    EXPECT_EQ(first.Map([](int) { return 0; }).GetCount(), 1);
    EXPECT_EQ(first.Where([](int x) { return x > 1; }).GetCount(), 2);
    EXPECT_EQ(first.Reduce(0, [](int x, int sum) { return sum + x; }), 6);
    auto copy = first;
    copy.Remove(1);
    EXPECT_TRUE(first.Contains(1));
    EXPECT_TRUE(copy.IsValid());
}

struct Task {
    int id{0};
    int priority{0};
    bool operator==(const Task& other) const { return id == other.id && priority == other.priority; }
};

struct TaskOrder {
    bool operator()(const Task& a, const Task& b) const { return a.priority < b.priority; }
};

TEST(Queue, DuplicatesPrioritiesRangesAndFunctions) {
    EXPECT_THROW(PriorityQueue<int>(nullptr), std::invalid_argument);
    EXPECT_EQ(PriorityQueue<int>::FromString("1 3 2").Peek(), 3);
    EXPECT_THROW(PriorityQueue<int>::FromString("bad"), std::invalid_argument);
    for (int kind = 0; kind < 2; ++kind) {
        SCOPED_TRACE(kind);
        UniquePtr<IPriorityStorage<Task>> taskStorage;
        UniquePtr<IPriorityStorage<int>> storage;
        if (kind == 0) {
            taskStorage = MakeUnique<TreePriorityStorage<Task, TaskOrder>>();
            storage = MakeUnique<TreePriorityStorage<int>>();
        } else {
            taskStorage = MakeUnique<HeapPriorityStorage<Task, TaskOrder>>();
            storage = MakeUnique<HeapPriorityStorage<int>>();
        }
        PriorityQueue<Task> tasks(Move(taskStorage));
        for (int i = 0; i < 100; ++i) tasks.Enqueue(Task{i, i % 3});
        EXPECT_TRUE(tasks.IsValid());
        EXPECT_TRUE(tasks.Contains(Task{7, 1}));
        int seen[100]{};
        int priority = 3;
        while (!tasks.IsEmpty()) {
            const Task task = tasks.Dequeue();
            EXPECT_LE(task.priority, priority);
            priority = task.priority;
            ++seen[task.id];
            ASSERT_TRUE(tasks.IsValid());
        }
        for (int count : seen) EXPECT_EQ(count, 1);
        PriorityQueue<int> queue(storage->CreateEmpty());
        for (int i = 1; i <= 5; ++i) queue.Enqueue(i);
        queue.Enqueue(3);
        EXPECT_EQ(Elements(queue), "5 4 3 3 2 1 ");
        EXPECT_EQ(Elements(queue.GetSubsequence(1, 3)), "4 3 3 ");
        EXPECT_TRUE(queue.ContainsSubsequence(queue.GetSubsequence(1, 3)));
        ArraySequence<int> absent;
        absent.Append(5); absent.Append(3);
        EXPECT_FALSE(queue.ContainsSubsequence(absent));
        EXPECT_TRUE(queue.ContainsSubsequence(ArraySequence<int>()));
        EXPECT_THROW(queue.GetSubsequence(-1, 0), std::out_of_range);
        EXPECT_EQ(queue.Map([](int) { return 1; }).GetCount(), 6);
        EXPECT_EQ(queue.Where([](int x) { return x > 3; }).GetCount(), 2);
        EXPECT_EQ(queue.Reduce(std::string(), [](int x, std::string acc) { return acc + WriteText(x); }), "543321");
        const auto split = queue.Split([](int x) { return x % 2 == 0; });
        EXPECT_EQ(split.key.GetCount() + split.value.GetCount(), 6);
        const IEnumerable<int>& source = queue;
        EXPECT_EQ(queue.Merge(source).GetCount(), 12);
        auto restored = PriorityQueue<int>::FromString(queue.ToString(), storage->CreateEmpty());
        EXPECT_EQ(Elements(restored), Elements(queue));
        auto copy = queue;
        auto enumerator = queue.GetEnumerator();
        EXPECT_THROW(enumerator->CurrentRef(), std::logic_error);
        while (enumerator->MoveNext()) {
            if (enumerator->CurrentRef() == 5) EXPECT_EQ(&enumerator->CurrentRef(), &queue.Peek());
            EXPECT_EQ(enumerator->CurrentRef(), copy.Dequeue());
        }
        EXPECT_THROW(enumerator->CurrentRef(), std::logic_error);
        EXPECT_EQ(queue.GetCount(), 6);
        EXPECT_THROW(copy.Dequeue(), std::out_of_range);
        PriorityQueue<int> moved(Move(queue));
        EXPECT_TRUE(queue.IsEmpty());
        queue.Enqueue(8);
        EXPECT_EQ(queue.Peek(), 8);
        copy = moved;
        restored = Move(copy);
        EXPECT_TRUE(copy.IsEmpty());
        EXPECT_EQ(Elements(restored), Elements(moved));
        auto reverse = [](int first, int second) { return first > second; };
        UniquePtr<IPriorityStorage<int>> minimumStorage;
        if (kind == 0) minimumStorage = MakeUnique<TreePriorityStorage<int, bool (*)(int, int)>>(+reverse);
        else minimumStorage = MakeUnique<HeapPriorityStorage<int, bool (*)(int, int)>>(+reverse);
        PriorityQueue<int> minimum(Move(minimumStorage));
        minimum.Enqueue(3); minimum.Enqueue(1); minimum.Enqueue(2);
        EXPECT_EQ(Elements(minimum), "1 2 3 ");
        EXPECT_EQ(minimum.Map([](int value) { return value + 1; }).Peek(), 2);
        EXPECT_EQ(minimum.Where([](int value) { return value > 1; }).Peek(), 2);
        EXPECT_EQ(minimum.Merge(minimum).Peek(), 1);
        const auto parts = minimum.Split([](int value) { return value != 2; });
        EXPECT_EQ(parts.key.Peek(), 1);
        EXPECT_EQ(parts.value.Peek(), 2);
        const auto minimumCopy = minimum;
        EXPECT_EQ(minimumCopy.Peek(), 1);
    }
}

TEST(Heap, BuildRemoveAndOrdering) {
    ArraySequence<int> source;
    for (int i = 0; i < 101; ++i) source.Append(i % 17);
    BinaryHeap<int> heap(source);
    EXPECT_TRUE(heap.IsValid());
    EXPECT_TRUE(heap.Remove(7));
    EXPECT_FALSE(heap.Remove(100));
    heap.Add(200);
    EXPECT_TRUE(heap.Contains(200));
    int previous = 200;
    while (!heap.IsEmpty()) {
        const int value = heap.Pop();
        EXPECT_LE(value, previous);
        previous = value;
        ASSERT_TRUE(heap.IsValid());
    }
    EXPECT_THROW(heap.Peek(), std::out_of_range);
    auto reverse = [](int a, int b) { return a > b; };
    BinaryHeap<int, bool (*)(int, int)> minimum(source, +reverse);
    EXPECT_EQ(minimum.Peek(), 0);
}

struct Resource {
    inline static int alive{0};
    Resource() { ++alive; }
    Resource(const Resource&) { ++alive; }
    Resource& operator=(const Resource&) = default;
    bool operator<(const Resource&) const { return false; }
    ~Resource() { --alive; }
};

TEST(Ownership, PointersArrayAndList) {
    DynamicArray<UniquePtr<Resource>> resources;
    resources.Append(MakeUnique<Resource>());
    resources.Append(MakeUnique<Resource>());
    resources.PopBack();
    EXPECT_EQ(Resource::alive, 1);
    resources.Clear();
    EXPECT_EQ(Resource::alive, 0);
    DynamicArray<std::string> values;
    values.Append("one"); values.Append("two");
    values.RemoveAt(0);
    EXPECT_EQ(values.Get(0), "two");
    EXPECT_THROW(values.Resize(-1), std::invalid_argument);
    ForwardList<int> list;
    list.PushBack(1); list.PushFront(0); list.PushBack(2);
    auto copy = list;
    EXPECT_TRUE(list.RemoveFirst([](int x) { return x == 2; }));
    list.PushBack(3);
    for (int value : copy) EXPECT_LE(value, 2);
    list.PopFront(); list.PopFront(); list.PopFront();
    EXPECT_TRUE(list.IsEmpty());
    list.PushBack(4);
    EXPECT_EQ(*list.begin(), 4);
    {
        BinaryTree<Resource> tree;
        tree.Add(Resource());
        auto copy = tree;
        tree.Clear();
        EXPECT_EQ(Resource::alive, 1);
        EXPECT_EQ(copy.GetCount(), 1);
    }
    EXPECT_EQ(Resource::alive, 0);
    {
        NaryTree<Resource> tree(2);
        for (int i = 0; i < 5; ++i) tree.Add(Resource());
        auto copy = tree;
        tree.Clear();
        EXPECT_EQ(Resource::alive, 5);
        int transformed = 0;
        EXPECT_THROW(copy.Map([&](const Resource& value) {
            if (++transformed == 3) throw std::runtime_error("stop");
            return value;
        }), std::runtime_error);
        EXPECT_EQ(Resource::alive, 5);
    }
    EXPECT_EQ(Resource::alive, 0);
    {
        PriorityQueue<Resource> queue(MakeUnique<HeapPriorityStorage<Resource>>());
        queue.Enqueue(Resource());
        auto copy = queue;
        queue.Clear();
        EXPECT_EQ(Resource::alive, 1);
    }
    EXPECT_EQ(Resource::alive, 0);
}

TEST(Values, RecordsFunctionsAndTiming) {
    const Student student = SampleStudent(4);
    EXPECT_TRUE(ReadText<Student>(WriteText(student)) == student);
    const Teacher teacher = SampleTeacher(5);
    EXPECT_TRUE(ReadText<Teacher>(WriteText(teacher)) == teacher);
    const Complex complex = SampleComplex(3);
    EXPECT_TRUE(ReadText<Complex>(WriteText(complex)) == complex);
    BinaryTree<int (*)(int), FunctionOrder> functions;
    functions.Add(Square); functions.Add(Negate); functions.Add(Increment);
    const auto restored = BinaryTree<int (*)(int), FunctionOrder>::FromString(functions.ToString(WriteFunction), ReadFunction, FunctionOrder());
    EXPECT_EQ((*restored.Find(Square))(3), 9);
    EXPECT_THROW(ReadFunction("unknown"), std::invalid_argument);
    BinaryTreeSet<Teacher> teachers;
    teachers.Add(teacher);
    EXPECT_TRUE(BinaryTreeSet<Teacher>::FromString(teachers.ToString()).Contains(teacher));
    BinaryTree<Complex> numbers;
    numbers.Add(complex);
    EXPECT_TRUE(*BinaryTree<Complex>::FromString(numbers.ToString()).Find(complex) == complex);
    PriorityQueue<Student> students(MakeUnique<TreePriorityStorage<Student, StudentPriority>>());
    students.Enqueue(student); students.Enqueue(student);
    const auto studentsCopy = PriorityQueue<Student>::FromString(students.ToString(), MakeUnique<TreePriorityStorage<Student, StudentPriority>>());
    EXPECT_EQ(studentsCopy.GetCount(), 2);
    EXPECT_TRUE(studentsCopy.Contains(student));
    int executed = 0;
    const auto complete = Measure(10, 1, [&](int) { ++executed; });
    EXPECT_EQ(complete.completed, executed);
    EXPECT_FALSE(complete.timedOut);
    const auto limited = Measure(100000, 0.000000001, [](int) {});
    EXPECT_TRUE(limited.timedOut);
    EXPECT_LT(limited.completed, 100000);
}
