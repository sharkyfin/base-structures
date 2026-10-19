#pragma once

#include <iomanip>
#include <istream>
#include <ostream>
#include <string>

struct Person {
    int id{0};
    std::string firstName;
    std::string lastName;
    int birthYear{0};

    bool operator<(const Person& other) const { return id < other.id; }
    bool operator==(const Person& other) const {
        return id == other.id && firstName == other.firstName && lastName == other.lastName && birthYear == other.birthYear;
    }
};

inline std::ostream& operator<<(std::ostream& output, const Person& person) {
    return output << person.id << ' ' << std::quoted(person.firstName) << ' ' << std::quoted(person.lastName) << ' ' << person.birthYear;
}

inline std::istream& operator>>(std::istream& input, Person& person) {
    return input >> person.id >> std::quoted(person.firstName) >> std::quoted(person.lastName) >> person.birthYear;
}
