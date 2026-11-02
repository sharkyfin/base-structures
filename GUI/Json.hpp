#pragma once

#include <ostream>
#include <string>

inline void WriteJsonString(std::ostream& output, const std::string& value) {
    const char* digits = "0123456789abcdef";
    output << '"';
    for (unsigned char character : value) {
        if (character == '"' || character == '\\') output << '\\' << character;
        else if (character < 32) output << "\\u00" << digits[character / 16] << digits[character % 16];
        else output << character;
    }
    output << '"';
}
