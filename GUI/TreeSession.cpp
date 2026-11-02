#include "GUI/TreeSession.hpp"

#include <random>
#include <sstream>
#include <stdexcept>

#include "GUI/Json.hpp"
#include "dictionaries/HashTable.hpp"

TreeSession::TreeSession() {
    const int values[]{40, 20, 65, 10, 30, 50, 80, 25, 35, 45, 55, 70, 90};
    for (int value : values) tree.Add(value);
    Capture("ready", "Добавь число, найди узел или запусти обход.");
}

void TreeSession::Capture(const std::string& action, const std::string& message,
                          const int* current, const int* related) {
    TreeFrame frame;
    frame.action = action;
    frame.message = message;
    frame.hasCurrent = current != nullptr;
    frame.hasRelated = related != nullptr;
    if (current) frame.current = *current;
    if (related) frame.related = *related;
    frame.visited = visited;
    tree.VisitNodes([&](const int& value, const int* left, const int* right, int) {
        frame.nodes.Append(TreeNodeView{value, left ? *left : 0, right ? *right : 0,
                                       left != nullptr, right != nullptr, 1, 0});
    });
    // During a rotation, ancestors have not updated their cached heights yet.
    // Recompute the displayed heights from the actual links in this snapshot.
    HashTable<int, int> heights;
    for (int i = frame.nodes.GetCount() - 1; i >= 0; --i) {
        TreeNodeView& node = frame.nodes.GetMutable(i);
        const int leftHeight = node.hasLeft ? *heights.Find(node.left) : 0;
        const int rightHeight = node.hasRight ? *heights.Find(node.right) : 0;
        node.height = 1 + (leftHeight > rightHeight ? leftHeight : rightHeight);
        node.balance = leftHeight - rightHeight;
        heights.Add(node.value, node.height);
    }
    frames.Append(Move(frame));
}

void TreeSession::OnTreeEvent(TreeEvent event, const int* current, const int* related) noexcept {
    if (recordingFailed) return;
    try {
        const std::string value = current ? std::to_string(*current) : "";
        const std::string target = std::to_string(argument);
        switch (event) {
            case TreeEvent::Compare: {
                std::string message = "Сравниваем " + target + " с узлом " + value + ".";
                if (argument < *current) message += " Идём в левое поддерево.";
                else if (argument > *current) message += " Идём в правое поддерево.";
                else message += " Значения совпали.";
                Capture("compare", message, current);
                break;
            }
            case TreeEvent::Inserted:
                Capture("insert", "Добавляем " + value + " на свободное место. Проверяем баланс на пути к корню.", current);
                break;
            case TreeEvent::Duplicate:
                Capture("duplicate", "Узел " + value + " уже есть. Повторное значение не добавляется.", current);
                break;
            case TreeEvent::Found:
                Capture("found", "Узел " + value + " найден.", current);
                break;
            case TreeEvent::Missing:
                Capture("missing", "Дошли до пустой ветви. Числа " + target + " в дереве нет.");
                break;
            case TreeEvent::Removing:
                Capture("removing", "Нашли " + value + ". Удаляем узел, сохраняя порядок дерева.", current);
                break;
            case TreeEvent::DetachedMinimum:
                Capture("detach", "Отделяем минимум " + value + " правого поддерева для замены удаляемого узла.");
                break;
            case TreeEvent::Removed:
                Capture("remove", "Узел " + target + " удалён. Восстанавливаем баланс предков.", current);
                break;
            case TreeEvent::Imbalance:
                Capture("imbalance", "У узла " + value + " разница высот больше 1. Нужен AVL-поворот.", current);
                break;
            case TreeEvent::RotatedLeft:
                Capture("rotate-left", "Левый поворот вокруг " + std::to_string(*related) + ": узел " + value + " поднимается выше.", current, related);
                break;
            case TreeEvent::RotatedRight:
                Capture("rotate-right", "Правый поворот вокруг " + std::to_string(*related) + ": узел " + value + " поднимается выше.", current, related);
                break;
        }
    } catch (...) {
        // Recording must not interrupt an operation halfway through balancing.
        recordingFailed = true;
    }
}

void TreeSession::Traverse(int order) {
    auto enumerator = tree.GetEnumerator(static_cast<Traversal>(order));
    while (enumerator->MoveNext()) {
        const int& value = enumerator->CurrentRef();
        visited.Append(value);
        Capture("visit", "Посещаем " + std::to_string(value) + ". Добавляем значение в результат обхода.", &value);
    }
}

void TreeSession::MakeRandom(int seed) {
    tree.Clear();
    std::mt19937 random(static_cast<unsigned int>(seed));
    std::uniform_int_distribution<int> number(5, 95);
    while (tree.GetCount() < 15) tree.Add(number(random));
}

void TreeSession::PrepareRotation(int choice) {
    const int examples[][12]{
        {50, 30, 70, 20, 40, 60, 80, 10, 25, 35, 45, 5},
        {50, 30, 70, 20, 40, 60, 80, 55, 65, 75, 85, 95},
        {50, 30, 70, 20, 40, 60, 80, 10, 25, 35, 45, 37},
        {50, 30, 70, 20, 40, 60, 80, 55, 65, 75, 85, 63}
    };
    tree.Clear();
    for (int i = 0; i < 11; ++i) tree.Add(examples[choice][i]);
    argument = examples[choice][11];
}

void TreeSession::Execute(const std::string& action, int value) {
    if (action != "add" && action != "remove" && action != "find" && action != "traverse"
        && action != "clear" && action != "random" && action != "preset") {
        throw std::invalid_argument("Неизвестная команда.");
    }
    if ((action == "add" || action == "remove" || action == "find") && (value < -999 || value > 999)) {
        throw std::invalid_argument("Введи целое число от −999 до 999.");
    }
    if (action == "traverse" && (value < 0 || value > 6)) throw std::invalid_argument("Неизвестный обход.");
    if (action == "preset" && (value < 0 || value > 3)) throw std::invalid_argument("Неизвестный пример поворота.");

    argument = value;
    frames.Clear();
    visited.Clear();
    recordingFailed = false;
    if (action == "preset") PrepareRotation(value);
    Capture("start", action == "preset" ? "Добавляем " + std::to_string(argument) + ", чтобы показать балансировку." : "Начальное состояние дерева.");

    tree.SetObserver(this);
    try {
        if (action == "add" || action == "preset") tree.Add(argument);
        else if (action == "remove") tree.Remove(value);
        else if (action == "find") tree.Find(value);
        else if (action == "traverse") Traverse(value);
        else if (action == "clear") tree.Clear();
        else if (action == "random") {
            tree.SetObserver(nullptr);
            MakeRandom(value);
        }
    } catch (...) {
        tree.SetObserver(nullptr);
        throw;
    }
    tree.SetObserver(nullptr);
    if (recordingFailed) throw std::runtime_error("Не удалось записать анимацию. Обнови страницу, чтобы увидеть дерево.");

    const std::string& lastAction = frames.Get(frames.GetCount() - 1).action;
    if (action == "find" || lastAction == "duplicate" || lastAction == "missing") return;
    const std::string message = action == "traverse" ? "Обход завершён. Все узлы добавлены в результат."
        : action == "clear" ? "Дерево очищено. Можно начать с первого числа."
        : action == "random" ? "Создано 15 узлов. Попробуй найти число или запустить обход."
        : "Операция завершена. Порядок ключей и AVL-баланс сохранены.";
    Capture("done", message);

    if (action == "preset") {
        int first = 0;
        while (first < frames.GetCount() && frames.Get(first).action != "imbalance") ++first;
        if (first < frames.GetCount()) {
            for (int i = 0; i < first; ++i) frames.RemoveAt(0);
        }
    }
}

const ArraySequence<TreeFrame>& TreeSession::GetFrames() const {
    return frames;
}

std::string TreeSession::ToJson() const {
    std::ostringstream output;
    output << "{\"frames\":[";
    for (int i = 0; i < frames.GetCount(); ++i) {
        if (i) output << ',';
        const TreeFrame& frame = frames.Get(i);
        output << "{\"action\":";
        WriteJsonString(output, frame.action);
        output << ",\"message\":";
        WriteJsonString(output, frame.message);
        output << ",\"current\":" << (frame.hasCurrent ? std::to_string(frame.current) : "null")
               << ",\"related\":" << (frame.hasRelated ? std::to_string(frame.related) : "null") << ",\"visited\":[";
        for (int j = 0; j < frame.visited.GetCount(); ++j) {
            if (j) output << ',';
            output << frame.visited.Get(j);
        }
        output << "],\"nodes\":[";
        for (int j = 0; j < frame.nodes.GetCount(); ++j) {
            if (j) output << ',';
            const TreeNodeView& node = frame.nodes.Get(j);
            output << "{\"value\":" << node.value
                   << ",\"left\":" << (node.hasLeft ? std::to_string(node.left) : "null")
                   << ",\"right\":" << (node.hasRight ? std::to_string(node.right) : "null")
                   << ",\"height\":" << node.height << ",\"balance\":" << node.balance << '}';
        }
        output << "]}";
    }
    output << "]}";
    return output.str();
}
