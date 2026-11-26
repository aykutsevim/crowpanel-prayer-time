#include "nvp/providers/json_stream.h"

#include <cstring>

namespace nvp::providers {
namespace {

bool is_ws(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
bool is_number_char(char c) {
    return (c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E';
}
int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

}  // namespace

const char* json_error_text(JsonError e) {
    switch (e) {
        case JsonError::None: return "none";
        case JsonError::Syntax: return "syntax";
        case JsonError::TooDeep: return "too deep";
        case JsonError::KeyTooLong: return "key too long";
        case JsonError::PathTooLong: return "path too long";
        case JsonError::Truncated: return "truncated";
        case JsonError::TrailingData: return "trailing data";
    }
    return "?";
}

JsonStream::JsonStream(JsonSink& sink) : sink_(sink) { allow_close_ = false; }

bool JsonStream::fail(JsonError e) {
    if (error_ == JsonError::None) error_ = e;
    state_ = State::Failed;
    return false;
}

void JsonStream::emit(JsonType t, const char* text, uint16_t length, bool overflow) {
    path_[path_len_] = '\0';
    JsonEvent e;
    e.type = t;
    e.path = path_;
    e.text = text;
    e.length = length;
    e.overflow = overflow;
    sink_.on_json(e);
}

// After a value: back to the container's path, then a comma or the container's end.
void JsonStream::end_value() {
    if (depth_ == 0) {
        path_len_ = 0;
        state_ = State::Done;
        return;
    }
    path_len_ = stack_[depth_ - 1].base;
    state_ = State::ExpectCommaOrEnd;
}

bool JsonStream::push(bool array) {
    if (depth_ >= kMaxDepth) return fail(JsonError::TooDeep);
    emit(array ? JsonType::BeginArray : JsonType::BeginObject, "", 0, false);
    stack_[depth_].array = array;
    stack_[depth_].base = path_len_;
    ++depth_;
    allow_close_ = true;
    state_ = array ? State::ExpectValue : State::ExpectKey;
    return true;
}

bool JsonStream::close_container(bool array) {
    if (depth_ == 0 || stack_[depth_ - 1].array != array) return fail(JsonError::Syntax);
    --depth_;
    path_len_ = stack_[depth_].base;
    emit(array ? JsonType::EndArray : JsonType::EndObject, "", 0, false);
    end_value();
    return true;
}

// The first byte of a value. In an array, the element's path is set first.
bool JsonStream::begin_value(char c) {
    if (depth_ > 0 && stack_[depth_ - 1].array) {
        uint16_t base = stack_[depth_ - 1].base;
        if (base + 2u > kMaxPath) return fail(JsonError::PathTooLong);
        path_[base] = '[';
        path_[base + 1] = ']';
        path_len_ = static_cast<uint16_t>(base + 2);
    }
    allow_close_ = false;
    switch (c) {
        case '{': return push(false);
        case '[': return push(true);
        case '"':
            string_is_key_ = false;
            str_len_ = 0;
            overflow_ = false;
            high_surrogate_ = 0;
            state_ = State::InString;
            return true;
        case 't':
            literal_ = "true";
            literal_type_ = JsonType::True;
            break;
        case 'f':
            literal_ = "false";
            literal_type_ = JsonType::False;
            break;
        case 'n':
            literal_ = "null";
            literal_type_ = JsonType::Null;
            break;
        default:
            if (c == '-' || (c >= '0' && c <= '9')) {
                num_len_ = 1;
                num_[0] = c;
                state_ = State::InNumber;
                return true;
            }
            return fail(JsonError::Syntax);
    }
    literal_pos_ = 1;
    state_ = State::InLiteral;
    return true;
}

void JsonStream::append_string_byte(char c) {
    if (str_len_ < kMaxString) {
        str_[str_len_++] = c;
    } else {
        overflow_ = true;
    }
}

void JsonStream::append_utf8(uint32_t cp) {
    if (cp < 0x80) {
        append_string_byte(static_cast<char>(cp));
    } else if (cp < 0x800) {
        append_string_byte(static_cast<char>(0xC0 | (cp >> 6)));
        append_string_byte(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        append_string_byte(static_cast<char>(0xE0 | (cp >> 12)));
        append_string_byte(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        append_string_byte(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        append_string_byte(static_cast<char>(0xF0 | (cp >> 18)));
        append_string_byte(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        append_string_byte(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        append_string_byte(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

// The closing quote of a key or a string value.
bool JsonStream::string_done() {
    if (high_surrogate_ != 0) {  // a lone high surrogate
        append_string_byte('?');
        high_surrogate_ = 0;
    }
    str_[str_len_] = '\0';
    if (!string_is_key_) {
        emit(JsonType::String, str_, str_len_, overflow_);
        end_value();
        return true;
    }
    if (overflow_) return fail(JsonError::KeyTooLong);
    uint16_t base = stack_[depth_ - 1].base;
    size_t need = base + (base > 0 ? 1u : 0u) + str_len_;
    if (need > kMaxPath) return fail(JsonError::PathTooLong);
    uint16_t at = base;
    if (base > 0) path_[at++] = '.';
    std::memcpy(path_ + at, str_, str_len_);
    path_len_ = static_cast<uint16_t>(at + str_len_);
    state_ = State::ExpectColon;
    return true;
}

bool JsonStream::step(char c) {
    switch (state_) {
        case State::ExpectValue:
            if (is_ws(c)) return true;
            if (c == ']' && allow_close_) return close_container(true);
            return begin_value(c);

        case State::ExpectKey:
            if (is_ws(c)) return true;
            if (c == '}' && allow_close_) return close_container(false);
            if (c != '"') return fail(JsonError::Syntax);
            allow_close_ = false;
            string_is_key_ = true;
            str_len_ = 0;
            overflow_ = false;
            high_surrogate_ = 0;
            state_ = State::InString;
            return true;

        case State::ExpectColon:
            if (is_ws(c)) return true;
            if (c != ':') return fail(JsonError::Syntax);
            state_ = State::ExpectValue;
            allow_close_ = false;
            return true;

        case State::ExpectCommaOrEnd: {
            if (is_ws(c)) return true;
            const bool array = stack_[depth_ - 1].array;
            if (c == ',') {
                allow_close_ = false;
                state_ = array ? State::ExpectValue : State::ExpectKey;
                return true;
            }
            if (c == (array ? ']' : '}')) return close_container(array);
            return fail(JsonError::Syntax);
        }

        case State::InString:
            if (c == '"') return string_done();
            if (c == '\\') {
                state_ = State::InEscape;
                return true;
            }
            if (static_cast<unsigned char>(c) < 0x20) return fail(JsonError::Syntax);
            if (high_surrogate_ != 0) {
                append_string_byte('?');
                high_surrogate_ = 0;
            }
            append_string_byte(c);
            return true;

        case State::InEscape: {
            char out = 0;
            switch (c) {
                case '"': out = '"'; break;
                case '\\': out = '\\'; break;
                case '/': out = '/'; break;
                case 'b': out = '\b'; break;
                case 'f': out = '\f'; break;
                case 'n': out = '\n'; break;
                case 'r': out = '\r'; break;
                case 't': out = '\t'; break;
                case 'u':
                    unicode_ = 0;
                    unicode_digits_ = 0;
                    state_ = State::InUnicode;
                    return true;
                default: return fail(JsonError::Syntax);
            }
            if (high_surrogate_ != 0) {
                append_string_byte('?');
                high_surrogate_ = 0;
            }
            append_string_byte(out);
            state_ = State::InString;
            return true;
        }

        case State::InUnicode: {
            int v = hex_value(c);
            if (v < 0) return fail(JsonError::Syntax);
            unicode_ = (unicode_ << 4) | static_cast<uint32_t>(v);
            if (++unicode_digits_ < 4) return true;
            state_ = State::InString;
            if (unicode_ >= 0xD800 && unicode_ <= 0xDBFF) {
                if (high_surrogate_ != 0) append_string_byte('?');
                high_surrogate_ = unicode_;
            } else if (unicode_ >= 0xDC00 && unicode_ <= 0xDFFF) {
                if (high_surrogate_ != 0) {
                    append_utf8(0x10000 + ((high_surrogate_ - 0xD800) << 10) + (unicode_ - 0xDC00));
                    high_surrogate_ = 0;
                } else {
                    append_string_byte('?');
                }
            } else {
                if (high_surrogate_ != 0) {
                    append_string_byte('?');
                    high_surrogate_ = 0;
                }
                append_utf8(unicode_);
            }
            return true;
        }

        case State::InNumber:
            if (is_number_char(c)) {
                if (num_len_ >= kMaxNumber) return fail(JsonError::Syntax);
                num_[num_len_++] = c;
                return true;
            }
            num_[num_len_] = '\0';
            emit(JsonType::Number, num_, num_len_, false);
            end_value();
            return step(c);  // the byte that ended the number

        case State::InLiteral:
            if (literal_[literal_pos_] != c) return fail(JsonError::Syntax);
            if (literal_[++literal_pos_] == '\0') {
                emit(literal_type_, "", 0, false);
                end_value();
            }
            return true;

        case State::Done:
            if (is_ws(c)) return true;
            return fail(JsonError::TrailingData);

        case State::Failed:
            return false;
    }
    return fail(JsonError::Syntax);
}

bool JsonStream::feed(const char* data, size_t n) {
    if (state_ == State::Failed) return false;
    for (size_t i = 0; i < n; ++i) {
        if (!step(data[i])) return false;
    }
    return true;
}

bool JsonStream::finish() {
    if (state_ == State::Failed) return false;
    if (state_ == State::InNumber && depth_ == 0) {
        num_[num_len_] = '\0';
        emit(JsonType::Number, num_, num_len_, false);
        end_value();
    }
    if (state_ == State::Done) return true;
    return fail(JsonError::Truncated);
}

}  // namespace nvp::providers
