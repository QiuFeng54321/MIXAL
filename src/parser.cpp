#include <cassert>
#include <iostream>
#include "parser.h"
#include "instructions.h"

namespace mixal {

enum class ParseState {
    // LOC OP ADDRESS,INDEX(FIELD)
    START,
    LOC,
    BEFORE_OP,
    OP,
    BEFORE_ADDRESS,
    // CONSTANT,
    ADDRESS,
    BEFORE_INDEX,
    INDEX,
    FIELD_OPEN,
    FIELD,
    FIELD_CLOSE,
    BEFORE_COMMENT,
    COMMENT,
    END,
};

#ifdef __DEBUG__
std::ostream& operator<<(std::ostream& os, ParseState c) {
    switch (c) {
    case ParseState::START: os << "START"; break;
    case ParseState::LOC: os << "LOC"; break;
    case ParseState::BEFORE_OP: os << "BEFORE_OP"; break;
    case ParseState::OP: os << "OP"; break;
    case ParseState::BEFORE_ADDRESS: os << "BEFORE_ADDRESS"; break;
    case ParseState::ADDRESS: os << "ADDRESS"; break;
    case ParseState::BEFORE_INDEX: os << "BEFORE_INDEX"; break;
    case ParseState::INDEX: os << "INDEX"; break;
    case ParseState::FIELD_OPEN: os << "FIELD_OPEN"; break;
    case ParseState::FIELD: os << "FIELD"; break;
    case ParseState::FIELD_CLOSE: os << "FIELD_CLOSE"; break;
    case ParseState::BEFORE_COMMENT: os << "BEFORE_COMMENT"; break;
    case ParseState::COMMENT: os << "COMMENT"; break;
    case ParseState::END: os << "END"; break;
    }
    return os;
}
#endif  // __DEBUG__

std::ostream& operator<<(std::ostream& os, ParsedType c) {
    switch (c) {
    case ParsedType::EMPTY: os << "EMPTY"; break;
    case ParsedType::INSTRUCTION: os << "INSTRUCTION"; break;
    }
    return os;
}

ParsedResult Parser::parseLine(const std::string& line, const std::string& lineSymbol, bool hasLocation) {
    const char END_CHAR = '#';
    const int INIT_INDEX = -1;
    ParsedResult result;
    result.parsedType = ParsedType::INSTRUCTION;
    auto state = hasLocation ? ParseState::START : ParseState::BEFORE_OP;
    int locationStart = INIT_INDEX,
        operationStart = INIT_INDEX,
        addressStart = INIT_INDEX,
        indexStart = INIT_INDEX,
        fieldStart = INIT_INDEX,
        commentStart = INIT_INDEX,
        defaultField = INIT_INDEX;
    std::unordered_map<std::string, AtomicValue> emptyDict;
    for (int i = 0; i <= static_cast<int>(line.size()); ++i) {
        char ch = i < static_cast<int>(line.size()) ? line[i] : END_CHAR;
        switch (state) {
        case ParseState::START:
            if (ch == ' ') {
                state = ParseState::BEFORE_OP;
            } else if (ch == '*') {
                state = ParseState::COMMENT;
                result.parsedType = ParsedType::EMPTY;
                commentStart = i;
            } else {
                state = ParseState::LOC;
                locationStart = i;
            }
            break;
        case ParseState::LOC:
            if (ch == ' ') {
                state = ParseState::BEFORE_OP;
                result.location = line.substr(locationStart, i - locationStart);
            } else if (!isalnum(ch)) {
                throw ParseError(i, "Unexpected character encountered while parsing location");
            }
            break;
        case ParseState::BEFORE_OP:
            if (ch == ' ') {
                continue;
            } else if (ch == END_CHAR) {
                if (locationStart != INIT_INDEX) {
                    throw ParseError(i, "No operation found after location");
                }
                state = ParseState::END;
                result.parsedType = ParsedType::EMPTY;
            } else if (isalnum(ch)) {
                state = ParseState::OP;
                operationStart = i;
            } else {
                throw ParseError(i, "Unexpected character encountered while finding operation");
            }
            break;
        case ParseState::OP:
            if (ch == ' ' || ch == END_CHAR) {
                if (ch == ' ') {
                    state = ParseState::BEFORE_ADDRESS;
                } else {
                    state = ParseState::END;
                }
                result.operation = line.substr(operationStart, i - operationStart);
                result.word.operation = static_cast<int>(Instructions::getInstructionCode(result.operation));
                defaultField = Instructions::getDefaultField(result.operation);
            } else if (!isalnum(ch)) {
                throw ParseError(i, "Unexpected character encountered while parsing operation");
            }
            break;
        case ParseState::BEFORE_ADDRESS:
            if (ch == ' ') {
                continue;
            } else if (ch == END_CHAR) {
                state = ParseState::END;
            } else if (Expression::isValidFirst(ch)) {
                state = ParseState::ADDRESS;
                result.word.sign = true;
                addressStart = i;
            } else {
                throw ParseError(i, "Unexpected character encountered while finding address");
            }
            break;
        case ParseState::ADDRESS:
            if (ch == ' ' || ch == ',' || ch == '(' || ch == END_CHAR) {
                if (ch == ' ') {
                    state = ParseState::BEFORE_COMMENT;
                } else if (ch == ',') {
                    state = ParseState::BEFORE_INDEX;
                } else if (ch == '(') {
                    state = ParseState::FIELD_OPEN;
                } else {
                    state = ParseState::END;
                }
                result.rawAddress = line.substr(addressStart, i - addressStart);
                try {
                    result.address.parse(result.rawAddress, lineSymbol);
                } catch (const ExpressionError& e) {
                    throw ParseError(addressStart + e.index(), e.what());
                }
                if (result.address.evaluate(emptyDict)) {
                    int32_t address = result.address.result().value;
                    if (abs(address) >= 4096) {
                        throw ParseError(i, "Address can not be represented in 2 bytes");
                    }
                    result.word.sign = result.address.result().negative;
                    result.word.address = static_cast<uint16_t>(abs(address));;
                }
            } else if (!Expression::isValidChar(ch)) {
                throw ParseError(i, "Unexpected character encountered while parsing address");
            }
            break;
        case ParseState::BEFORE_INDEX:
            if (Expression::isValidFirst(ch)) {
                state = ParseState::INDEX;
                indexStart = i;
            } else if (ch == END_CHAR) {
                throw ParseError(i, "No index found after comma");
            } else {
                throw ParseError(i, "Unexpected character encountered while finding index");
            }
            break;
        case ParseState::INDEX:
            if (ch == ' ' || ch == '(' || ch == END_CHAR) {
                if (ch == ' ') {
                    state = ParseState::BEFORE_COMMENT;
                } else if (ch == '(') {
                    state = ParseState::FIELD_OPEN;
                } else {
                    state = ParseState::END;
                }
                result.rawIndex = line.substr(indexStart, i - indexStart);
                try {
                    result.index.parse(result.rawIndex, lineSymbol);
                } catch (const ExpressionError& e) {
                    throw ParseError(addressStart + e.index(), e.what());
                }
                if (result.index.evaluate(emptyDict)) {
                    int32_t index = result.index.result().value;
                    if (index < 0 || 6 < index) {
                        throw ParseError(i, "Invalid index value: " + std::to_string(result.word.index));
                    }
                    result.word.index = static_cast<uint8_t>(index);
                }
            } else if (!Expression::isValidChar(ch)) {
                throw ParseError(i, "Unexpected character encountered while parsing index");
            }
            break;
        case ParseState::FIELD_OPEN:
            if (Expression::isValidFirst(ch)) {
                state = ParseState::FIELD;
                fieldStart = i;
            } else {
                throw ParseError(i, "Unexpected character encountered while parsing modification");
            }
            break;
        case ParseState::FIELD:
            if (ch == ')') {
                state = ParseState::FIELD_CLOSE;
                result.rawField = line.substr(fieldStart, i - fieldStart);
                try {
                    result.field.parse(result.rawField, lineSymbol);
                } catch (const ExpressionError& e) {
                    throw ParseError(addressStart + e.index(), e.what());
                }
                if (result.field.evaluate(emptyDict)) {
                    int32_t field = result.field.result().value;
                    if (defaultField >= 0 && field != defaultField) {
                        throw ParseError(i, "The given field value does not match the default one: " +
                                            std::to_string(field) + " != " + std::to_string(defaultField));
                    }
                    if (field < 0 || 64 <= field) {
                        throw ParseError(i, "Invalid field value: " + std::to_string(field));
                    }
                    result.word.field = static_cast<uint8_t>(field);
                }
            } else if (!Expression::isValidChar(ch)) {
                throw ParseError(i, "Unexpected character encountered while parsing index");
            }
            break;
        case ParseState::FIELD_CLOSE:
            if (ch == ' ' || ch == END_CHAR) {
                if (ch == ' ') {
                    state = ParseState::BEFORE_COMMENT;
                } else {
                    state = ParseState::END;
                }
            } else {
                throw ParseError(i, "Unexpected character encountered while parsing digital modification");
            }
            break;
        case ParseState::BEFORE_COMMENT:
            if (ch == END_CHAR) {
                state = ParseState::END;
            } else if (ch != ' ') {
                state = ParseState::COMMENT;
                commentStart = i;
            }
            break;
        case ParseState::COMMENT:
            if (ch == END_CHAR) {
                state = ParseState::END;
                result.comment = line.substr(commentStart, i - commentStart);
            }
            break;
        case ParseState::END:
            assert(false);
        }
    }
    if (defaultField >= 0) {
        result.word.field = defaultField;
    }
    assert(state == ParseState::END);
    return result;
}

};  // namespace mixal
