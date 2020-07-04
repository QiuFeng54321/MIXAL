#include <stdexcept>
#include <string>
#include <iostream>
#include "memory.h"

namespace mixal {

uint16_t CHAR_CODES[] = {
    ' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
    0xb4, 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
    0x2da, 0x2dd, 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '.', ',', '(', ')', '+', '-', '*', '/', '=', '$',
    '<', '>', '@', ';', ':', 0x201a
};

bool ComputerWord::operator==(const ComputerWord& word) const {
    return negative == word.negative && byte1 == word.byte1 &&
           byte2 == word.byte2 && byte3 == word.byte3 &&
           byte4 == word.byte4 && byte5 == word.byte5;
}

std::ostream& operator<<(std::ostream& out, const ComputerWord& word) {
    out << (word.negative ? '-' : '+');
    for (int i = 1; i <= 5; ++i) {
        out << ' ' << static_cast<int>(word[i]);
    }
    return out;
}

std::string ComputerWord::getBytesString() const {
    std::string result;
    result += negative ? '-' : '+';
    for (int i = 1; i <= 5; ++i) {
        result += ' ';
        if ((*this)[i] < 10) {
            result += ' ';
        }
        result += std::to_string(static_cast<int>((*this)[i]));
    }
    return result;
}

uint8_t ComputerWord::operator[](int index) const {
    if (index <= 0 || index > 5) {
        throw std::runtime_error("Invalid index for a word: " + std::to_string(index));
    }
    switch (index) {
    case 1: return byte1;
    case 2: return byte2;
    case 3: return byte3;
    case 4: return byte4;
    default: return byte5;
    }
}

uint8_t& ComputerWord::operator[](int index) {
    if (index <= 0 || index > 5) {
        throw std::runtime_error("Invalid index for a word: " + std::to_string(index));
    }
    switch (index) {
    case 1: return byte1;
    case 2: return byte2;
    case 3: return byte3;
    case 4: return byte4;
    default: return byte5;
    }
}

uint16_t ComputerWord::bytes2(int index1, int index2) const {
    int16_t high = static_cast<int16_t>(static_cast<uint8_t>((*this)[index1]));
    int16_t low = static_cast<int16_t>(static_cast<uint8_t>((*this)[index2]));
    return high * 64 + low;
}

uint16_t ComputerWord::bytes12() const {
    return bytes2(1, 2);
}

uint16_t ComputerWord::bytes23() const {
    return bytes2(2, 3);
}

uint16_t ComputerWord::bytes34() const {
    return bytes2(3, 4);
}

uint16_t ComputerWord::bytes45() const {
    return bytes2(4, 5);
}

int32_t ComputerWord::value() const {
    int32_t value = static_cast<int32_t>(byte1 << 24) |
                    static_cast<int32_t>(byte2 << 18) |
                    static_cast<int32_t>(byte3 << 12) |
                    static_cast<int32_t>(byte4 << 6) |
                    static_cast<int32_t>(byte5);
    return negative ? -value : value;
}

int16_t ComputerWord::addressValue() const {
    int16_t value = static_cast<int16_t>(this->bytes12());
    if (negative) {
        value = -value;
    }
    return value;
}

void ComputerWord::setAddress(int16_t address) {
    negative = false;
    if (address < 0) {
        negative = true;
        address = -address;
    }
    byte1 = static_cast<uint8_t>(address / 64);
    byte2 = static_cast<uint8_t>(address % 64);
}

void ComputerWord::setAddress(bool negative, uint16_t address) {
    this->negative = negative;
    byte1 = static_cast<uint8_t>(address / 64);
    byte2 = static_cast<uint8_t>(address % 64);
}

std::string ComputerWord::getCharacters() const {
    std::string chars;
    for (int i = 1; i <= 5; ++i) {
        if (getAt(i) < CHAR_CODES_NUM) {
            uint16_t code = CHAR_CODES[getAt(i)];
            if (code < 127) {
                chars += static_cast<char>(code);
            } else {
                chars += ' ';
            }
        } else {
            chars += ' ';
        }
    }
    return chars;
}

void ComputerWord::set(int32_t value) {
    if (value > 0) {
        negative = false;
    } else if (value < 0) {
        negative = true;
        value = -value;
    }
    for (int i = 5; i >= 1; --i) {
        set(i, static_cast<uint8_t>(value & ((1 << 6) - 1)));
        value >>= 6;
    }
}

void ComputerWord::set(const std::string& chars) {
    if (chars.length() != 5) {
        throw std::runtime_error("Invalid length of characters for a word: " + chars);
    }
    negative = false;
    for (int i = 0; i < 5; ++i) {
        (*this)[i + 1] = 0;
        for (int j = 0; j < CHAR_CODES_NUM; ++j) {
            if (static_cast<uint16_t>(chars[i]) == CHAR_CODES[j]) {
                (*this)[i + 1] = j;
                break;
            }
        }
    }
}

void ComputerWord::set(int index, uint8_t val) {
    if (index <= 0 || index > 5) {
        throw std::runtime_error("Invalid index for a word: " + std::to_string(index));
    }
    switch (index) {
    case 1: byte1 = val; break;
    case 2: byte2 = val; break;
    case 3: byte3 = val; break;
    case 4: byte4 = val; break;
    default: byte5 = val;
    }
}

void ComputerWord::set(bool negative, uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4, uint8_t byte5) {
    this->negative = negative;
    this->byte1 = byte1;
    this->byte2 = byte2;
    this->byte3 = byte3;
    this->byte4 = byte4;
    this->byte5 = byte5;
}

void ComputerWord::set(bool negative, uint16_t bytes12, uint8_t byte3, uint8_t byte4, uint8_t byte5) {
    this->negative = negative;
    this->byte1 = static_cast<uint8_t>(bytes12 / 64);
    this->byte2 = static_cast<uint8_t>(bytes12 % 64);
    this->byte3 = byte3;
    this->byte4 = byte4;
    this->byte5 = byte5;
}

};  // namespace mixal
