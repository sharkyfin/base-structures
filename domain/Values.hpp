#pragma once

#include "domain/Person.hpp"
#include "core/Text.hpp"

struct Complex {
    double real{0};
    double imaginary{0};
    bool operator<(const Complex& other) const {
        return real < other.real || (real == other.real && imaginary < other.imaginary);
    }
    bool operator==(const Complex& other) const { return real == other.real && imaginary == other.imaginary; }
};

inline std::ostream& operator<<(std::ostream& output, const Complex& value) { return output << value.real << ' ' << value.imaginary; }
inline std::istream& operator>>(std::istream& input, Complex& value) { return input >> value.real >> value.imaginary; }

struct Student {
    Person person;
    double grade{0};
    bool operator<(const Student& other) const { return person < other.person; }
    bool operator==(const Student& other) const { return person == other.person && grade == other.grade; }
};

inline std::ostream& operator<<(std::ostream& output, const Student& value) { return output << value.person << ' ' << value.grade; }
inline std::istream& operator>>(std::istream& input, Student& value) { return input >> value.person >> value.grade; }

struct Teacher {
    Person person;
    int experience{0};
    bool operator<(const Teacher& other) const { return person < other.person; }
    bool operator==(const Teacher& other) const { return person == other.person && experience == other.experience; }
};

inline std::ostream& operator<<(std::ostream& output, const Teacher& value) { return output << value.person << ' ' << value.experience; }
inline std::istream& operator>>(std::istream& input, Teacher& value) { return input >> value.person >> value.experience; }

struct StudentPriority {
    bool operator()(const Student& a, const Student& b) const { return a.grade < b.grade; }
};

struct TeacherPriority {
    bool operator()(const Teacher& a, const Teacher& b) const { return a.experience < b.experience; }
};

inline int Square(int value) { return value * value; }
inline int Negate(int value) { return -value; }
inline int Increment(int value) { return value + 1; }

inline std::string WriteFunction(int (*function)(int)) {
    if (function == Square) return "square";
    if (function == Negate) return "negate";
    if (function == Increment) return "increment";
    throw std::invalid_argument("Unknown function.");
}

inline int (*ReadFunction(const std::string& text))(int) {
    if (text == "square") return Square;
    if (text == "negate") return Negate;
    if (text == "increment") return Increment;
    throw std::invalid_argument("Use square, negate or increment.");
}

struct FunctionOrder {
    bool operator()(int (*a)(int), int (*b)(int)) const { return WriteFunction(a) < WriteFunction(b); }
};

inline int SampleInt(int index) { return index + 1; }
inline double SampleDouble(int index) { return static_cast<double>(index + 1) / 10; }
inline std::string SampleString(int index) { return "value " + std::to_string(index + 1); }
inline Complex SampleComplex(int index) { return Complex{static_cast<double>(index + 1), static_cast<double>(index % 7)}; }
inline Student SampleStudent(int index) { return Student{Person{index + 1, "Ivan", "Petrov", 2000 + index % 10}, 2.0 + index % 4}; }
inline Teacher SampleTeacher(int index) { return Teacher{Person{index + 1, "Anna", "Ivanova", 1960 + index % 40}, index % 30}; }
inline int (*SampleFunction(int index))(int) {
    int (*functions[])(int){Square, Negate, Increment};
    return functions[index % 3];
}
