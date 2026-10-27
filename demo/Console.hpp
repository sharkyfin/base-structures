#pragma once

#include <iostream>
#include <random>
#include <string>

#include "benchmark/Benchmark.hpp"
#include "domain/Values.hpp"
#include "trees/BinaryTree.hpp"

inline std::string ReadLine(const std::string& prompt);

inline int ReadInt(const std::string& prompt);

inline void ShowText(const std::string& text);

inline Traversal ReadTraversal();

template <class Action>
void RunMenu(const char* menu, Action action);

template <class T, class Writer>
void Show(const ICollectible<T>& source, Writer write, UniquePtr<IEnumerator<T>> enumerator = {},
    const std::string& details = {});

template <class T, class Reader, class Generator>
ArraySequence<T> ReadValues(Reader read, Generator generate);

template <class T, class Collection, class Reader, class Writer, class Compare>
void FunctionalOperation(int operation, Collection& collection, Reader read, Writer write, Compare compare);

template <class T, class Reader, class Writer, class Generator, class Compare>
void TreeMenu(Reader read, Writer write, Generator generate, Compare compare);

template <class T, class Reader, class Writer, class Generator, class Compare>
void SetMenu(Reader read, Writer write, Generator generate, Compare compare);

template <class T, class Reader, class Writer, class Generator, class Compare>
void QueueMenu(Reader read, Writer write, Generator generate, Compare compare);

template <class T, class Reader, class Writer, class Generator, class Compare>
void HeapMenu(Reader read, Writer write, Generator generate, Compare compare);

template <class T, class Reader, class Writer, class Generator, class Compare = Less<T>, class PriorityCompare = Compare>
void SelectStructure(Reader read, Writer write, Generator generate, Compare compare = Compare(), PriorityCompare priority = PriorityCompare());

inline void RunDemo();

inline void RunConsole();

#include "Console.tpp"
