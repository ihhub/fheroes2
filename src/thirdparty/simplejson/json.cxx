/* MIT License

Copyright (c) 2018 Gregory Eslinger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Code source: https://github.com/gregjesl/simpleson
*/

/*! \file json.cpp
 * \brief Simpleson source file
 */

#include "json.h"
#include <cctype>
#include <cstring>
#include <cassert>

#define DEBUG 0
/*! \brief Checks for an empty string
 *
 * @param str The string to check
 * @return True if the string is empty, false if the string is not empty
 * @warning The string must be null-terminated for this macro to work
 */
#define EMPTY_STRING(str) (*str == '\0')

/*! \brief Moves a pointer to the first character that is not white space
 *
 * @param str The pointer to move
 */
#define SKIP_WHITE_SPACE(str) { const char *next = json::parsing::tlws(str); str = next; }

/*! \brief Determines if the end character of serialized JSON is encountered
 *
 * @param obj The JSON object or array that is being written to
 * @param index The pointer to the character to be checked
 */
#define END_CHARACTER_ENCOUNTERED(obj, index) (obj.is_array() ? *index == ']' : *index == '}')

/*! \brief Determines if the supplied character is a digit
 *
 * @param input The character to be tested
 */
#define IS_DIGIT(input) (input >= '0' && input <= '9')

/*! \brief Format used for integer to string conversion */
const char * INT_FORMAT = "%i";

/*! \brief Format used for unsigned integer to string conversion */
const char * UINT_FORMAT = "%u";

/*! \brief Format used for long integer to string conversion */
const char * LONG_FORMAT = "%li";

/*! \brief Format used for unsigned long integer to string conversion */
const char * ULONG_FORMAT = "%lu";

/*! \brief Format used for character to string conversion */
const char * CHAR_FORMAT = "%c";

/*! \brief Format used for floating-point number to string conversion */
const char * FLOAT_FORMAT = "%f";

/*! \brief Format used for double floating-opint number to string conversion */
const char * DOUBLE_FORMAT = "%lf";

const char* json::parsing::tlws(const char *input)
{
    const char *output = input;
    while(!EMPTY_STRING(output) && std::isspace(*output)) output++;
    return output;
}

json::jtype::jtype json::jtype::peek(const char input)
{
    switch (input)
    {
    case '[':
        return json::jtype::jarray;
    case '"':
        return json::jtype::jstring;
    case '{':
        return json::jtype::jobject;
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return json::jtype::jnumber;
    case 't':
    case 'f':
        return json::jtype::jbool;
    case 'n':
        return json::jtype::jnull;
    default:
        return json::jtype::not_valid;
    }
}

json::jtype::jtype json::jtype::detect(const char *input)
{
    const char *start = json::parsing::tlws(input);
    return json::jtype::peek(*start);
}

void json::reader::clear()
{
    std::string::clear();
    if(this->sub_reader != NULL) {
        delete this->sub_reader;
        this->sub_reader = NULL;
    }
    this->read_state = 0;
}

json::reader::push_result json::reader::push(const char next)
{
    // Check for opening whitespace
    if(this->length() == 0 && std::isspace(next)) return reader::ACCEPTED;

    // Get the type
    const json::jtype::jtype type = json::jtype::peek(this->length() > 0 ? this->front() : next);

    // Store the return
    reader::push_result result = reader::REJECTED;

    #if DEBUG
    const size_t start_length = this->length();
    #endif

    switch(type)
    {
    case json::jtype::jarray:
        result = this->push_array(next);
        break;
    case json::jtype::jbool:
        result = this->push_boolean(next);
        assert(result != WHITESPACE);
        break;
    case json::jtype::jnull:
        result = this->push_null(next);
        assert(result != WHITESPACE);
        break;
    case json::jtype::jnumber:
        result = this->push_number(next);
        assert(result != WHITESPACE);
        break;
    case json::jtype::jobject:
        result = this->push_object(next);
        break;
    case json::jtype::jstring:
        result = this->push_string(next);
        assert(result != WHITESPACE);
        break;
    case json::jtype::not_valid:
        result = reader::REJECTED;
        break;
    default:
        result = reader::REJECTED;
        break;
    }

    // Verify the expected length change
    #if DEBUG
    if(result == ACCEPTED) assert(this->length() - start_length == 1);
    else assert(this->length() == start_length);
    #endif

    // Return the result
    return result;
}

bool json::reader::is_valid() const
{
    switch (this->type())
    {
    case jtype::jarray:
        return this->get_state<array_reader_enum>() == ARRAY_CLOSED;
    case jtype::jbool:
        if(this->length() < 4) return false;
        if(this->length() == 4 && *this == "true") return true;
        if(this->length() == 5 && *this == "false") return true;
        return false;
    case jtype::jnull:
        return (this->length() == 4 && *this == "null");
    case jtype::jnumber:
        switch (this->get_state<number_reader_enum>())
        {
        case NUMBER_ZERO:
        case NUMBER_INTEGER_DIGITS:
        case NUMBER_FRACTION_DIGITS:
        case NUMBER_EXPONENT_DIGITS:
            return true;
        default:
            return false;
        }
    case jtype::jobject:
        return this->get_state<object_reader_enum>() == OBJECT_CLOSED;
    case jtype::jstring:
        return this->get_state<string_reader_enum>() == STRING_CLOSED;
    case jtype::not_valid:
        return false;
    default:
        break;
    }
    throw std::logic_error("Unexpected return");
}

bool json::is_control_character(const char input)
{
    switch (input)
    {
    case 'b':
    case 'f':
    case 'n':
    case 'r':
    case 't':
    case '"':
    case '\\':
        return true;
    default:
        return false;
    }
}

bool json::is_hex_digit(const char input)
{
    return IS_DIGIT(input) || (input >= 'a' && input <= 'f') || (input >= 'A' && input <= 'F');
}

json::reader::push_result json::reader::push_string(const char next)
{
    const string_reader_enum state = this->get_state<string_reader_enum>();
    switch (state)
    {
    case STRING_EMPTY:
        assert(this->length() == 0);
        if(next == '"') {
            assert(this->length() == 0);
            this->push_back(next);
            this->set_state(STRING_OPENING_QUOTE);
            return ACCEPTED;
        }
        return REJECTED;
    case STRING_OPENING_QUOTE:
        assert(this->length() == 1);
        this->set_state(STRING_OPEN);
        // Fall through deliberate
        [[fallthrough]];
    case STRING_OPEN:
        assert(this->length() > 0);
        switch (next)
        {
        case '\\':
            this->set_state(STRING_ESCAPED);
            break;
        case '"':
            this->set_state(STRING_CLOSED);
            break;
        default:
            // No state change
            break;
        }
        this->push_back(next);
        return ACCEPTED;
    case STRING_ESCAPED:
        if(is_control_character(next)) {
            this->set_state(STRING_OPEN);
            this->push_back(next);
            return ACCEPTED;
        } else if(next == 'u') {
            this->set_state(STRING_CODE_POINT_START);
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    case STRING_CODE_POINT_START:
        assert(this->back() == 'u');
        if(!is_hex_digit(next)) return REJECTED;
        this->push_back(next);
        this->set_state(STRING_CODE_POINT_1);
        return ACCEPTED;
    case STRING_CODE_POINT_1:
        assert(is_hex_digit(this->back()));
        if(!is_hex_digit(next)) return REJECTED;
        this->push_back(next);
        this->set_state(STRING_CODE_POINT_2);
        return ACCEPTED;
    case STRING_CODE_POINT_2:
        assert(is_hex_digit(this->back()));
        if(!is_hex_digit(next)) return REJECTED;
        this->push_back(next);
        this->set_state(STRING_CODE_POINT_3);
        return ACCEPTED;
    case STRING_CODE_POINT_3:
        assert(is_hex_digit(this->back()));
        if(!is_hex_digit(next)) return REJECTED;
        this->push_back(next);
        this->set_state(STRING_OPEN);
        return ACCEPTED;
    case STRING_CLOSED:
        return REJECTED;
    default:
        break;
    }
    throw std::logic_error("Unexpected return");
}

json::reader::push_result json::reader::push_array(const char next)
{
    const array_reader_enum state = this->get_state<array_reader_enum>();

    switch (state)
    {
    case ARRAY_EMPTY:
        assert(this->sub_reader == NULL);
        if(next == '[') {
            this->set_state(ARRAY_OPEN_BRACKET);
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    case ARRAY_OPEN_BRACKET:
        assert(this->sub_reader == NULL);
        if(std::isspace(next)) return WHITESPACE;
        if(next == ']') {
            this->set_state(ARRAY_CLOSED);
            this->push_back(next);
            return ACCEPTED;
        }
        begin_reading_value:
        if(json::jtype::peek(next) == json::jtype::not_valid) return REJECTED;
        this->sub_reader = new reader();
        this->set_state(ARRAY_READING_VALUE);
        // Fall-through deliberate
        [[fallthrough]];
    case ARRAY_READING_VALUE:
        assert(this->sub_reader != NULL);
        if(this->sub_reader->is_valid() && std::isspace(next)) return WHITESPACE;
        switch (this->sub_reader->push(next))
        {
        case ACCEPTED:
            return ACCEPTED;
        case WHITESPACE:
            return WHITESPACE;
        case REJECTED:
            switch (next)
            {
            case ']':
                if(!this->sub_reader->is_valid()) return REJECTED;
                this->append(this->sub_reader->readout());
                delete this->sub_reader;
                this->sub_reader = NULL;
                this->push_back(next);
                this->set_state(ARRAY_CLOSED);
                return ACCEPTED;
            case ',':
                if(!this->sub_reader->is_valid()) return REJECTED;
                this->append(this->sub_reader->readout());
                delete this->sub_reader;
                this->sub_reader = NULL;
                this->push_back(next);
                this->set_state(ARRAY_AWAITING_NEXT_LINE);
                return ACCEPTED;
            default:
                return REJECTED;
            }
        default:
            return REJECTED;
        }
        // This point should not be reached
        break;
    case ARRAY_AWAITING_NEXT_LINE:
        if(std::isspace(next)) return WHITESPACE;
        goto begin_reading_value;
    case ARRAY_CLOSED:
        return REJECTED;
    default:
        break;
    }
    throw std::logic_error("Unexpected return");
}

json::reader::push_result json::reader::push_object(const char next)
{
    const object_reader_enum state = this->get_state<object_reader_enum>();

    switch (state)
    {
    case OBJECT_EMPTY:
        assert(this->sub_reader == NULL);
        if(next == '{') {
            this->set_state(OBJECT_OPEN_BRACE);
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    case OBJECT_OPEN_BRACE:
        assert(this->sub_reader == NULL);
        if(next == '}') {
            this->set_state(OBJECT_CLOSED);
            this->push_back(next);
            return ACCEPTED;
        }
        // Fall-through deliberate
        [[fallthrough]];
    case OBJECT_AWAITING_NEXT_LINE:
        if(std::isspace(next)) return WHITESPACE;
        if(next != '"') return REJECTED;
        this->sub_reader = new kvp_reader();
        #if DEBUG
        assert(
        #endif
        this->sub_reader->push(next)
        #if DEBUG
        == ACCEPTED);
        #else
        ;
        #endif
        this->set_state(OBJECT_READING_ENTRY);
        return ACCEPTED;
    case OBJECT_READING_ENTRY:
        assert(this->sub_reader != NULL);
        switch (this->sub_reader->push(next))
        {
        case ACCEPTED:
            return ACCEPTED;
        case WHITESPACE:
            return WHITESPACE;
        case REJECTED:
            if(!this->sub_reader->is_valid()) return REJECTED;
            if(std::isspace(next)) return WHITESPACE;
            switch (next)
            {
            case '}':
                this->append(this->sub_reader->readout());
                delete this->sub_reader;
                this->sub_reader = NULL;
                this->push_back(next);
                this->set_state(OBJECT_CLOSED);
                return ACCEPTED;
            case ',':
                this->append(this->sub_reader->readout());
                delete this->sub_reader;
                this->sub_reader = NULL;
                this->push_back(next);
                this->set_state(OBJECT_AWAITING_NEXT_LINE);
                return ACCEPTED;
            default:
                return REJECTED;
            }
        default:
            return REJECTED;
        }
        // This point should never be reached
        break;
    case OBJECT_CLOSED:
        return REJECTED;
    default:
        break;
    }
    throw std::logic_error("Unexpected return");
}

json::reader::push_result json::reader::push_number(const char next)
{
    const number_reader_enum state = this->get_state<number_reader_enum>();
    switch (state)
    {
    case NUMBER_EMPTY:
        assert(this->length() == 0);
        if(next == '-') {
            this->set_state(NUMBER_OPEN_NEGATIVE);
            this->push_back(next);
            return ACCEPTED;
        } else if(IS_DIGIT(next)) {
            this->set_state(next == '0' ? NUMBER_ZERO : NUMBER_INTEGER_DIGITS);
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    case NUMBER_OPEN_NEGATIVE:
        if(IS_DIGIT(next)) {
            this->set_state(next == '0' ? NUMBER_ZERO : NUMBER_INTEGER_DIGITS);
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    case NUMBER_INTEGER_DIGITS:
        assert(IS_DIGIT(this->back()));
        if(IS_DIGIT(next)) {
            this->push_back(next);
            return ACCEPTED;
        }
        // Fall-through deliberate
        [[fallthrough]];
    case NUMBER_ZERO:
        switch (next)
        {
        case '.':
            this->set_state(NUMBER_DECIMAL);
            this->push_back(next);
            return ACCEPTED;
        case 'e':
        case 'E':
            this->set_state(NUMBER_EXPONENT);
            this->push_back(next);
            return ACCEPTED;
        default:
            return REJECTED;
        }
    case NUMBER_DECIMAL:
        assert(this->back() == '.');
        if(IS_DIGIT(next)) {
            this->set_state(NUMBER_FRACTION_DIGITS);
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    case NUMBER_FRACTION_DIGITS:
        assert(IS_DIGIT(this->back()));
        if(IS_DIGIT(next)) {
            this->push_back(next);
            return ACCEPTED;
        } else if(next == 'e' || next == 'E') {
            this->set_state(NUMBER_EXPONENT);
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    case NUMBER_EXPONENT:
        assert(this->back() == 'e' || this->back() == 'E');
        if(next == '+' || next == '-') {
            this->set_state(NUMBER_EXPONENT_SIGN);
            this->push_back(next);
            return ACCEPTED;
        }
        // Fall-through deliberate
        [[fallthrough]];
    case NUMBER_EXPONENT_SIGN:
    case NUMBER_EXPONENT_DIGITS:
        if(IS_DIGIT(next)) {
            this->set_state(NUMBER_EXPONENT_DIGITS);
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    default:
        break;
    }
    throw std::logic_error("Unexpected return");
}

json::reader::push_result json::reader::push_boolean(const char next)
{
    const char *str_true = "true";
    const char *str_false = "false";
    const char *str = NULL;

    if(this->length() == 0) {
        switch (next)
        {
        case 't':
        case 'f':
            this->push_back(next);
            return ACCEPTED;
        default:
            return REJECTED;
        }
    }

    // Determine which string to use
    switch (this->at(0))
    {
    case 't':
        str = str_true;
        break;
    case 'f':
        str = str_false;
        break;
    default:
        throw json::parsing_error("Unexpected state");
    }
    assert(str == str_true || str == str_false);

    // Push the value
    if(this->length() < strlen(str) && str[this->length()] == next) {
        this->push_back(next);
        return ACCEPTED;
    }
    return REJECTED;
}

json::reader::push_result json::reader::push_null(const char next)
{
    switch (this->length())
    {
    case 0:
        if(next == 'n') {
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    case 1:
        if(next == 'u') {
            this->push_back(next);
            return ACCEPTED;
        }
        return REJECTED;
    case 2:
    case 3:
        if(next == 'l') {
            this->push_back(next);
            return ACCEPTED;
        }
        // Fall through
    case 4:
        return REJECTED;
    default:
        throw json::parsing_error("Unexpected state");
    }
}

json::reader::push_result json::kvp_reader::push(const char next)
{
    if(this->_key.length() == 0) {
        if(std::isspace(next)) return WHITESPACE;
        if(next == '"') {
            this->_key.push(next);
            assert(this->_key.type() == json::jtype::jstring);
            assert(this->_key.length() == 1);
            return ACCEPTED;
        }
        return REJECTED;
    } else if (!this->_key.is_valid()) {
        return this->_key.push(next);
    }

    // At this point the key should be valid
    assert(this->_key.is_valid());

    if(!this->_colon_read) {
        if(std::isspace(next)) return WHITESPACE;
        if(next == ':') {
            this->_colon_read = true;
            return ACCEPTED;
        }
        return REJECTED;
    }

    // At this point the colon should be read
    assert(this->_colon_read);

    // Check for a fresh start
    if(reader::length() == 0 && std::isspace(next))
    {
        assert(reader::get_state<char>() == 0);
        return WHITESPACE;
    }
    return reader::push(next);
}

std::string json::kvp_reader::readout() const
{
    return this->_key.readout() + ":" + reader::readout();
}

std::string json::parsing::read_digits(const char *input)
{
    // Trim leading white space
    const char *index = json::parsing::tlws(input);

    // Initialize the result
    std::string result;

    // Loop until all digits are read
    while (
        !EMPTY_STRING(index) &&
        (
            *index == '0' ||
            *index == '1' ||
            *index == '2' ||
            *index == '3' ||
            *index == '4' ||
            *index == '5' ||
            *index == '6' ||
            *index == '7' ||
            *index == '8' ||
            *index == '9'
            )
        )
    {
        result += *index;
        index++;
    }

    // Return the result
    return result;
}

std::string json::parsing::decode_string(const char *input)
{
    const char *index = input;
    std::string result;

    if(*index != '"') throw json::parsing_error("Expecting opening quote");
    index++;
    bool escaped = false;
    // Loop until the end quote is found
    while(!(!escaped && *index == '"'))
    {
        if(escaped)
        {
            switch (*index)
            {
            case '"':
            case '\\':
            case '/':
                result += *index;
                break;
            case 'b':
                result += '\b';
                break;
            case 'f':
                result += '\f';
                break;
            case 'n':
                result += '\n';
                break;
            case 'r':
                result += '\r';
                break;
            case 't':
                result += '\t';
                break;
            case 'u':
                // #todo Unicode support
                index += 4;
                break;
            default:
                throw json::parsing_error("Expected control character");
            }
            escaped = false;
        } else if(*index == '\\') {
            escaped = true;
        } else {
            result += *index;
        }
        index++;
    }
    return result;
}

std::string json::parsing::encode_string(const char *input)
{
    std::string result = "\"";

    while (!EMPTY_STRING(input))
    {
        switch (*input)
        {
        case '"':
        case '\\':
        case '/':
            result += "\\";
            result += *input;
            break;
        case '\b':
            result += "\\b";
            break;
        case '\f':
            result += "\\f";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            result += *input;
            break;
        }
        input++;
    }
    result += '\"';
    return result;
}

json::parsing::parse_results json::parsing::parse(const char *input)
{
    // Strip white space
    const char *index = json::parsing::tlws(input);

    // Validate input
    if (EMPTY_STRING(index)) throw json::parsing_error("Input was only whitespace");

    // Initialize the output
    json::parsing::parse_results result;
    result.type = json::jtype::not_valid;

    // Initialize the reader
    json::reader stream;

    // Iterate
    while(!EMPTY_STRING(input) && stream.push(*index) != json::reader::REJECTED)
    {
        index++;
    }

    if(stream.is_valid()) {
        result.value = stream.readout();
        result.type = stream.type();
    }
    result.remainder = index;

    return result;
}

std::vector<std::string> json::parsing::parse_array(const char *input)
{
    // Initalize the result
    std::vector<std::string> result;

    const char *index = json::parsing::tlws(input);
    if (*index != '[') throw json::parsing_error("Input was not an array");
    index++;
    SKIP_WHITE_SPACE(index);
    if (*index == ']')
    {
        return result;
    }
    const char error[] = "Input was not properly formated";
    while (!EMPTY_STRING(index))
    {
        SKIP_WHITE_SPACE(index);
        json::parsing::parse_results parse_results = json::parsing::parse(index);
        if (parse_results.type == json::jtype::not_valid) throw json::parsing_error(error);
        if(parse_results.type == json::jtype::jstring) {
            result.push_back(json::parsing::decode_string(parse_results.value.c_str()));
        } else {
            result.push_back(parse_results.value);
        }
        index = json::parsing::tlws(parse_results.remainder);
        if (*index == ']') break;
        if (*index == ',') index++;
    }
    if (*index != ']') throw json::parsing_error(error);
    index++;
    return result;
}

json::jobject::entry::operator int() const { return this->get_number<int>(INT_FORMAT); }
json::jobject::entry::operator unsigned int() const { return this->get_number<unsigned int>(UINT_FORMAT); }
json::jobject::entry::operator long() const { return this->get_number<long>(LONG_FORMAT); }
json::jobject::entry::operator unsigned long() const { return this->get_number<unsigned long>(ULONG_FORMAT); }
json::jobject::entry::operator char() const { return this->get_number<char>(CHAR_FORMAT); }
json::jobject::entry::operator float() const { return this->get_number<float>(FLOAT_FORMAT); }
json::jobject::entry::operator double() const { return this->get_number<double>(DOUBLE_FORMAT); }

json::jobject::entry::operator std::vector<int>() const { return this->get_number_array<int>(INT_FORMAT); }
json::jobject::entry::operator std::vector<unsigned int>() const { return this->get_number_array<unsigned int>(UINT_FORMAT); }
json::jobject::entry::operator std::vector<long>() const { return this->get_number_array<long>(LONG_FORMAT); }
json::jobject::entry::operator std::vector<unsigned long>() const { return this->get_number_array<unsigned long>(ULONG_FORMAT); }
json::jobject::entry::operator std::vector<char>() const { return this->get_number_array<char>(CHAR_FORMAT); }
json::jobject::entry::operator std::vector<float>() const { return this->get_number_array<float>(FLOAT_FORMAT); }
json::jobject::entry::operator std::vector<double>() const { return this->get_number_array<double>(DOUBLE_FORMAT); }

void json::jobject::proxy::set_array(const std::vector<std::string> &values, const bool wrap)
{
    std::string value = "[";
    for (size_t i = 0; i < values.size(); i++)
    {
        if (wrap) value += json::parsing::encode_string(values[i].c_str()) + ",";
        else value += values[i] + ",";
    }
    if(values.size() > 0) value.erase(value.size() - 1, 1);
    value += "]";
    this->sink.set(key, value);
}

json::jobject json::jobject::parse(const char *input)
{
    const char error[] = "Input is not a valid object";
    const char *index = json::parsing::tlws(input);
    json::jobject result;
    json::reader stream;
    switch (*index)
    {
    case '{':
        // Result is already an object
        break;
    case '[':
        result = json::jobject(true);
        break;
    default:
        throw json::parsing_error(error);
        break;
    }
    index++;
    SKIP_WHITE_SPACE(index);
    if (EMPTY_STRING(index)) throw json::parsing_error(error);

    while (!EMPTY_STRING(index) && !END_CHARACTER_ENCOUNTERED(result, index))
    {
        // Get key
        kvp entry;

        if(!result.is_array()) {
            json::parsing::parse_results key = json::parsing::parse(index);
            if (key.type != json::jtype::jstring || key.value == "") throw json::parsing_error(error);
            entry.first = json::parsing::decode_string(key.value.c_str());
            index = key.remainder;

            // Get value
            SKIP_WHITE_SPACE(index);
            if (*index != ':') throw json::parsing_error(error);
            index++;
        }

        SKIP_WHITE_SPACE(index);
        json::parsing::parse_results value = json::parsing::parse(index);
        if (value.type == json::jtype::not_valid) throw json::parsing_error(error);
        entry.second = value.value;
        index = value.remainder;

        // Clean up
        SKIP_WHITE_SPACE(index);
        if (*index != ',' && !END_CHARACTER_ENCOUNTERED(result, index)) throw json::parsing_error(error);
        if (*index == ',') index++;
        result += entry;

    }
    if (EMPTY_STRING(index) || !END_CHARACTER_ENCOUNTERED(result, index)) throw json::parsing_error(error);
    index++;
    return result;
}

json::key_list_t json::jobject::list_keys() const
{
    // Initialize the result
    key_list_t result;

    // Return an empty list if the object is an array
    if(this->is_array()) return result;

    for(size_t i = 0; i < this->data.size(); i++)
    {
        result.push_back(this->data.at(i).first);
    }
    return result;
}

void json::jobject::set(const std::string &ikey, const std::string &ivalue)
{
    if(this->array_flag) throw json::invalid_key(ikey);
    for (size_t i = 0; i < this->size(); i++)
    {
        if (this->data.at(i).first == ikey)
        {
            this->data.at(i).second = ivalue;
            return;
        }
    }
    kvp lentry;
    lentry.first = ikey;
    lentry.second = ivalue;
    this->data.push_back(lentry);
}

void json::jobject::remove(const std::string &key)
{
    for (size_t i = 0; i < this->size(); i++)
    {
        if (this->data.at(i).first == key)
        {
            this->remove(i);
        }
    }
}

json::jobject::operator std::string() const
{
    if (is_array()) {
        if (this->size() == 0) return "[]";
        std::string result = "[";
        for (size_t i = 0; i < this->size(); i++)
        {
            result += this->data.at(i).second + ",";
        }
        result.erase(result.size() - 1, 1);
        result += "]";
        return result;
    } else {
        if (this->size() == 0) return "{}";
        std::string result = "{";
        for (size_t i = 0; i < this->size(); i++)
        {
            result += json::parsing::encode_string(this->data.at(i).first.c_str()) + ":" + this->data.at(i).second + ",";
        }
        result.erase(result.size() - 1, 1);
        result += "}";
        return result;
    }
}

std::string json::jobject::pretty(unsigned int indent_level) const
{
    std::string result = "";
    for(unsigned int i = 0; i < indent_level; i++) result += "\t";
    if (is_array()) {
        if(this->size() == 0) {
            result += "[]";
            return result;
        }
        result += "[\n";
        for (size_t i = 0; i < this->size(); i++)
        {
            switch(json::jtype::peek(*this->data.at(i).second.c_str())) {
                case json::jtype::jarray:
                case json::jtype::jobject:
                    result += json::jobject::parse(this->data.at(i).second).pretty(indent_level + 1);
                    break;
                default:
                    for(unsigned int j = 0; j < indent_level + 1; j++) result += "\t";
                    result += this->data.at(i).second;
                    break;
            }

            result += ",\n";
        }
        result.erase(result.size() - 2, 1);
        for(unsigned int i = 0; i < indent_level; i++) result += "\t";
        result += "]";
    } else {
        if(this->size() == 0) {
            result += "{}";
            return result;
        }
        result += "{\n";
        for (size_t i = 0; i < this->size(); i++)
        {
            for(unsigned int j = 0; j < indent_level + 1; j++) result += "\t";
            result += "\"" + this->data.at(i).first + "\": ";
            switch(json::jtype::peek(*this->data.at(i).second.c_str())) {
                case json::jtype::jarray:
                case json::jtype::jobject:
                    result += std::string(json::parsing::tlws(json::jobject::parse(this->data.at(i).second).pretty(indent_level + 1).c_str()));
                    break;
                default:
                    result += this->data.at(i).second;
                    break;
            }

            result += ",\n";
        }
        result.erase(result.size() - 2, 1);
        for(unsigned int i = 0; i < indent_level; i++) result += "\t";
        result += "}";
    }
    return result;
}
