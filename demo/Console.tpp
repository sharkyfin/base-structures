#pragma once

inline std::string ReadLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) throw std::runtime_error("Ввод завершён.");
    return line;
}

inline int ReadInt(const std::string& prompt) { return ReadText<int>(ReadLine(prompt)); }

inline void ShowText(const std::string& text) {
    std::cout << text.substr(0, 2000) << (text.length() > 2000 ? "..." : "") << '\n';
}

inline Traversal ReadTraversal() {
    const int order = ReadInt("Обход: 0 КЛП, 1 КПЛ, 2 ЛПК, 3 ЛКП, 4 ПЛК, 5 ПКЛ, 6 по уровням: ");
    if (order < 0 || order > 6) throw std::invalid_argument("Неверный обход.");
    return static_cast<Traversal>(order);
}

template <class Action>
void RunMenu(const char* menu, Action action) {
    while (std::cin) {
        try {
            std::cout << '\n' << menu << '\n';
            const int command = ReadInt("> ");
            if (command == 0) return;
            action(command);
        } catch (const std::exception& error) {
            if (!std::cin) return;
            std::cout << "Ошибка: " << error.what() << '\n';
        }
    }
}

template <class T, class Writer>
void Show(const ICollectible<T>& source, Writer write, UniquePtr<IEnumerator<T>> enumerator,
    const std::string& details) {
    if (!enumerator) enumerator = source.GetEnumerator();
    int printed = 0;
    while (printed < 20 && enumerator->MoveNext()) {
        std::cout << write(enumerator->CurrentRef()) << '\n';
        ++printed;
    }
    if (printed < source.GetCount()) std::cout << "...\n";
    std::cout << "Элементов: " << source.GetCount() << details << '\n';
}

template <class T, class Reader, class Generator>
ArraySequence<T> ReadValues(Reader read, Generator generate) {
    const int mode = ReadInt("1 — вручную, 2 — сгенерировать: ");
    if (mode != 1 && mode != 2) throw std::invalid_argument("Неверный способ ввода.");
    const int count = ReadInt("Количество: ");
    if (count < 0) throw std::invalid_argument("Количество должно быть неотрицательным.");
    ArraySequence<T> values;
    values.Reserve(count);
    for (int i = 0; i < count; ++i) values.Append(mode == 1 ? read(ReadLine("Элемент: ")) : generate(i));
    if (mode == 2) {
        const unsigned int seed = static_cast<unsigned int>(ReadInt("Seed: "));
        std::mt19937 random(seed);
        for (int i = count - 1; i > 0; --i) {
            std::uniform_int_distribution<int> position(0, i);
            Swap(values.GetMutable(i), values.GetMutable(position(random)));
        }
    }
    return values;
}

template <class T, class Collection, class Reader, class Writer, class Compare>
void FunctionalOperation(int operation, Collection& collection, Reader read, Writer write, Compare compare) {
    if (operation == 1) {
        const T from = read(ReadLine("Заменить значение: "));
        const T to = read(ReadLine("Новое значение: "));
        collection = collection.Map([&](const T& value) { return value == from ? to : value; });
    } else if (operation == 2) {
        const T threshold = read(ReadLine("Оставить значения не меньше: "));
        collection = collection.Where([&](const T& value) { return !compare(value, threshold); });
    } else {
        const std::string result = collection.Reduce(std::string(), [&](const T& value, std::string acc) {
            if (acc.length() < 400) acc += write(value) + " | ";
            return acc;
        });
        std::cout << result.substr(0, 400) << (result.length() > 400 ? "..." : "") << '\n';
    }
}

template <class T, class Reader, class Writer, class Generator, class Compare>
void TreeMenu(Reader read, Writer write, Generator generate, Compare compare) {
    BinaryTree<T, Compare> tree(compare);
    RunMenu("Дерево: 1 заполнить, 2 добавить, 3 удалить, 4 найти, 5 обход, 6 поддерево\n"
        "7 путь, 8 Map, 9 Where, 10 Reduce, 11 слияние\n"
        "12 в строку, 13 из строки, 14 вхождение поддерева, 0 назад", [&](int command) {
        if (command == 1 || command == 11 || command == 14) {
            const auto values = ReadValues<T>(read, generate);
            BinaryTree<T, Compare> other(compare);
            for (int i = 0; i < values.GetCount(); ++i) other.Add(values.Get(i));
            if (command == 1) tree = Move(other);
            else if (command == 11) tree = tree.Merge(other);
            else std::cout << (tree.ContainsSubtree(other) ? "Входит\n" : "Не входит\n");
        } else if (command == 2) std::cout << (tree.Add(read(ReadLine("Значение: "))) ? "Добавлено\n" : "Уже есть\n");
        else if (command == 3) std::cout << (tree.Remove(read(ReadLine("Значение: "))) ? "Удалено\n" : "Не найдено\n");
        else if (command == 4) std::cout << (tree.Find(read(ReadLine("Значение: "))) ? "Найдено\n" : "Не найдено\n");
        else if (command == 5) Show(tree, write, tree.GetEnumerator(ReadTraversal()), ", высота: " + std::to_string(tree.GetHeight()));
        else if (command == 6) Show(tree.ExtractSubtree(read(ReadLine("Корень поддерева: "))), write);
        else if (command == 7) {
            const int relative = ReadInt("0 — от корня, 1 — от элемента: ");
            if (relative == 0) std::cout << write(tree.GetByPath(ReadLine("Путь L/R, пустой — корень: "))) << '\n';
            else if (relative == 1) {
                const T start = read(ReadLine("Начальный элемент: "));
                std::cout << write(tree.GetByPath(start, ReadLine("Путь L/R: "))) << '\n';
            } else throw std::invalid_argument("Неверный вид пути.");
        } else if (command >= 8 && command <= 10) FunctionalOperation<T>(command - 7, tree, read, write, compare);
        else if (command == 12) ShowText(tree.ToString(write));
        else if (command == 13) tree = BinaryTree<T, Compare>::FromString(ReadLine("Значения через пробелы: "), read, compare);
        else throw std::invalid_argument("Нет такой команды.");
    });
}

template <class T, class Reader, class Writer, class Generator, class Compare>
void SetMenu(Reader read, Writer write, Generator generate, Compare compare) {
    BinaryTreeSet<T, Compare> set(compare);
    RunMenu("Множество: 1 заполнить, 2 добавить, 3 удалить, 4 найти, 5 показать\n"
        "6 объединение, 7 пересечение, 8 разность, 9 включение, 10 равенство\n"
        "11 Map, 12 Where, 13 Reduce, 14 в строку, 15 из строки, 0 назад", [&](int command) {
        if (command == 1 || (command >= 6 && command <= 10)) {
            const auto values = ReadValues<T>(read, generate);
            BinaryTreeSet<T, Compare> other(compare);
            for (int i = 0; i < values.GetCount(); ++i) other.Add(values.Get(i));
            if (command == 1) set = Move(other);
            else if (command == 6) set = set | other;
            else if (command == 7) set = set & other;
            else if (command == 8) set = set - other;
            else if (command == 9) std::cout << (set <= other ? "Включено\n" : "Не включено\n");
            else std::cout << (set == other ? "Равны\n" : "Не равны\n");
        } else if (command == 2) set.Add(read(ReadLine("Значение: ")));
        else if (command == 3) std::cout << (set.Remove(read(ReadLine("Значение: "))) ? "Удалено\n" : "Не найдено\n");
        else if (command == 4) std::cout << (set.Contains(read(ReadLine("Значение: "))) ? "Найдено\n" : "Не найдено\n");
        else if (command == 5) Show(set, write);
        else if (command >= 11 && command <= 13) FunctionalOperation<T>(command - 10, set, read, write, compare);
        else if (command == 14) ShowText(set.ToString(write));
        else if (command == 15) set = BinaryTreeSet<T, Compare>::FromString(ReadLine("Значения через пробелы: "), read, compare);
        else throw std::invalid_argument("Нет такой команды.");
    });
}

template <class T, class Reader, class Writer, class Generator, class Compare>
void QueueMenu(Reader read, Writer write, Generator generate, Compare compare) {
    const int kind = ReadInt("Хранилище очереди: 1 AVL, 2 куча: ");
    if (kind != 1 && kind != 2) throw std::invalid_argument("Неверное хранилище.");
    const auto makeStorage = [&]() -> UniquePtr<IPriorityStorage<T>> {
        if (kind == 1) return MakeUnique<TreePriorityStorage<T, Compare>>(compare);
        return MakeUnique<HeapPriorityStorage<T, Compare>>(compare);
    };
    PriorityQueue<T> queue(makeStorage());
    RunMenu("Очередь: 1 заполнить, 2 добавить, 3 извлечь, 4 вершина, 5 показать, 6 найти\n"
        "7 диапазон, 8 вхождение последовательности, 9 слияние, 10 разделение\n"
        "11 Map, 12 Where, 13 Reduce, 14 в строку, 15 из строки, 0 назад", [&](int command) {
        if (command == 1 || command == 8 || command == 9) {
            const auto values = ReadValues<T>(read, generate);
            if (command == 8) std::cout << (queue.ContainsSubsequence(values) ? "Входит\n" : "Не входит\n");
            else {
                PriorityQueue<T> other(makeStorage());
                for (int i = 0; i < values.GetCount(); ++i) other.Enqueue(values.Get(i));
                if (command == 1) queue = Move(other);
                else queue = queue.Merge(other);
            }
        } else if (command == 2) queue.Enqueue(read(ReadLine("Значение: ")));
        else if (command == 3) std::cout << write(queue.Dequeue()) << '\n';
        else if (command == 4) std::cout << write(queue.Peek()) << '\n';
        else if (command == 5) Show(queue, write);
        else if (command == 6) std::cout << (queue.Contains(read(ReadLine("Значение: "))) ? "Найдено\n" : "Не найдено\n");
        else if (command == 7) {
            const int first = ReadInt("От (с нуля): ");
            const int last = ReadInt("До включительно: ");
            Show(queue.GetSubsequence(first, last), write);
        } else if (command == 10) {
            const T threshold = read(ReadLine("Граница: "));
            const auto parts = queue.Split([&](const T& value) { return !compare(value, threshold); });
            std::cout << "Не меньше границы:\n"; Show(parts.key, write);
            std::cout << "Остальные:\n"; Show(parts.value, write);
        } else if (command >= 11 && command <= 13) FunctionalOperation<T>(command - 10, queue, read, write, compare);
        else if (command == 14) ShowText(queue.ToString(write));
        else if (command == 15) queue = PriorityQueue<T>::FromString(ReadLine("Значения через пробелы: "), read, makeStorage());
        else throw std::invalid_argument("Нет такой команды.");
    });
}

template <class T, class Reader, class Writer, class Generator, class Compare>
void HeapMenu(Reader read, Writer write, Generator generate, Compare compare) {
    BinaryHeap<T, Compare> heap(compare);
    RunMenu("Куча: 1 заполнить, 2 добавить, 3 извлечь, 4 вершина, 5 массив, 6 найти, 7 удалить, 0 назад", [&](int command) {
        if (command == 1) heap = BinaryHeap<T, Compare>(ReadValues<T>(read, generate), compare);
        else if (command == 2) heap.Add(read(ReadLine("Значение: ")));
        else if (command == 3) std::cout << write(heap.Pop()) << '\n';
        else if (command == 4) std::cout << write(heap.Peek()) << '\n';
        else if (command == 5) Show(heap, write);
        else if (command == 6) std::cout << (heap.Contains(read(ReadLine("Значение: "))) ? "Найдено\n" : "Не найдено\n");
        else if (command == 7) std::cout << (heap.Remove(read(ReadLine("Значение: "))) ? "Удалено\n" : "Не найдено\n");
        else throw std::invalid_argument("Нет такой команды.");
    });
}

template <class T, class Reader, class Writer, class Generator, class Compare, class PriorityCompare>
void SelectStructure(Reader read, Writer write, Generator generate, Compare compare, PriorityCompare priority) {
    RunMenu("1 AVL, 2 множество, 3 очередь, 4 куча, 0 назад", [&](int command) {
        if (command == 1) TreeMenu<T>(read, write, generate, compare);
        else if (command == 2) SetMenu<T>(read, write, generate, compare);
        else if (command == 3) QueueMenu<T>(read, write, generate, priority);
        else if (command == 4) HeapMenu<T>(read, write, generate, priority);
        else throw std::invalid_argument("Нет такой команды.");
    });
}

inline void RunDemo() {
    BinaryTree<int> tree;
    const int values[]{4, 2, 6, 1, 3, 5, 7};
    for (int value : values) tree.Add(value);
    const char* names[]{"КЛП", "КПЛ", "ЛПК", "ЛКП", "ПЛК", "ПКЛ", "По уровням"};
    std::cout << "AVL, высота " << tree.GetHeight() << '\n';
    for (int order = 0; order < 7; ++order) {
        std::cout << names[order] << ": ";
        auto enumerator = tree.GetEnumerator(static_cast<Traversal>(order));
        while (enumerator->MoveNext()) std::cout << enumerator->CurrentRef() << ' ';
        std::cout << '\n';
    }
    std::cout << "Строка: " << tree.ToString() << '\n';
    BinaryTreeSet<int> first;
    BinaryTreeSet<int> second;
    first.Add(1); first.Add(2); second.Add(2); second.Add(3);
    std::cout << "Объединение:\n"; Show(first | second, WriteText<int>);
    PriorityQueue<Student> queue(MakeUnique<HeapPriorityStorage<Student, StudentPriority>>());
    queue.Enqueue(SampleStudent(0)); queue.Enqueue(SampleStudent(1)); queue.Enqueue(SampleStudent(5));
    std::cout << "Очередь студентов на куче по оценке, равные приоритеты сохраняются:\n";
    while (!queue.IsEmpty()) std::cout << WriteText(queue.Dequeue()) << '\n';
    BinaryHeap<int> heap(tree);
    std::cout << "Куча, извлечение: ";
    while (!heap.IsEmpty()) std::cout << heap.Pop() << ' ';
    std::cout << '\n';
}

inline void RunConsole() {
    RunMenu("Тип: 1 int, 2 double, 3 строка, 4 комплексное, 5 студент, 6 преподаватель, 7 функция\n"
        "8 бенчмарк, 9 демонстрация, 0 выход", [](int command) {
        switch (command) {
            case 1: SelectStructure<int>(ReadText<int>, WriteText<int>, SampleInt); break;
            case 2: SelectStructure<double>(ReadText<double>, WriteText<double>, SampleDouble); break;
            case 3: SelectStructure<std::string>(ReadText<std::string>, WriteText<std::string>, SampleString); break;
            case 4:
                std::cout << "Ввод: действительная и мнимая части.\n";
                SelectStructure<Complex>(ReadText<Complex>, WriteText<Complex>, SampleComplex); break;
            case 5:
                std::cout << "Ввод: ID \"имя\" \"фамилия\" год оценка. Дерево по ID, очередь по оценке.\n";
                SelectStructure<Student>(ReadText<Student>, WriteText<Student>, SampleStudent, Less<Student>(), StudentPriority()); break;
            case 6:
                std::cout << "Ввод: ID \"имя\" \"фамилия\" год стаж. Дерево по ID, очередь по стажу.\n";
                SelectStructure<Teacher>(ReadText<Teacher>, WriteText<Teacher>, SampleTeacher, Less<Teacher>(), TeacherPriority()); break;
            case 7:
                std::cout << "Функции: square, negate, increment. Порядок по имени.\n";
                SelectStructure<int (*)(int)>(ReadFunction, WriteFunction, SampleFunction, FunctionOrder()); break;
            case 8: {
                const int count = ReadInt("Максимальный N (10000..100000000): ");
                const double seconds = ReadText<double>(ReadLine("Лимит одной серии, секунды: "));
                RunBenchmarks(count, seconds);
                break;
            }
            case 9: RunDemo(); break;
            default: throw std::invalid_argument("Нет такой команды.");
        }
    });
}
