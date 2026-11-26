// A streaming JSON tokenizer (spec 006 research R2, FR-003, 05-architecture §3).
//
// SAX style: fed arbitrary chunks (a token may be split between chunks), it reports every value
// with its key path, for example `data.times[].imsak`. Object members are joined with '.', array
// elements are written `[]`. It never holds the document: fixed buffers, no heap.
//
// Limits: keys, numbers, depth and the path have hard limits (an error). A string value longer
// than kMaxString is reported truncated with `overflow` set, so a parser can reject it when it
// needs that field and ignore it otherwise (an answer may carry long text a parser never reads).
// Malformed, truncated or trailing input ends the parse with an error.
#pragma once

#include <cstddef>
#include <cstdint>

namespace nvp::providers {

enum class JsonType : uint8_t { BeginObject, EndObject, BeginArray, EndArray, String, Number, True, False, Null };

struct JsonEvent {
    JsonType type = JsonType::Null;
    const char* path = "";  // never null; "" for the top-level value
    const char* text = "";  // String and Number: the value (NUL-terminated); "" otherwise
    uint16_t length = 0;    // bytes in text
    bool overflow = false;  // String: longer than kMaxString; text holds the first kMaxString bytes
};

class JsonSink {
public:
    virtual ~JsonSink() = default;
    virtual void on_json(const JsonEvent& e) = 0;
};

enum class JsonError : uint8_t {
    None,
    Syntax,       // an unexpected byte
    TooDeep,      // nesting deeper than kMaxDepth
    KeyTooLong,   // a key longer than kMaxString
    PathTooLong,  // the key path longer than kMaxPath
    Truncated,    // finish() before the value was complete
    TrailingData  // bytes other than white space after the value
};

const char* json_error_text(JsonError e);

class JsonStream {
public:
    static constexpr size_t kMaxString = 64;
    static constexpr size_t kMaxDepth = 8;
    static constexpr size_t kMaxPath = 128;
    static constexpr size_t kMaxNumber = 32;

    explicit JsonStream(JsonSink& sink);

    // Feeds the next bytes. Returns false once an error has occurred (later calls do nothing).
    bool feed(const char* data, size_t n);
    // The end of the input: true when exactly one complete value was read.
    bool finish();

    JsonError error() const { return error_; }
    bool complete() const { return state_ == State::Done; }

private:
    enum class State : uint8_t {
        ExpectValue,
        ExpectKey,
        ExpectColon,
        ExpectCommaOrEnd,
        InString,
        InEscape,
        InUnicode,
        InNumber,
        InLiteral,
        Done,
        Failed
    };
    struct Frame {
        bool array = false;
        uint16_t base = 0;  // length of the container's own path
    };

    bool step(char c);
    bool begin_value(char c);
    void end_value();
    bool push(bool array);
    bool close_container(bool array);
    bool string_done();
    void append_string_byte(char c);
    void append_utf8(uint32_t cp);
    bool fail(JsonError e);
    void emit(JsonType t, const char* text, uint16_t length, bool overflow);

    JsonSink& sink_;
    State state_ = State::ExpectValue;
    JsonError error_ = JsonError::None;
    bool allow_close_ = false;  // ']' or '}' allowed right after '[' or '{'
    bool string_is_key_ = false;
    bool overflow_ = false;
    uint8_t depth_ = 0;
    Frame stack_[kMaxDepth];
    char path_[kMaxPath + 1] = {};
    uint16_t path_len_ = 0;
    char str_[kMaxString + 1] = {};
    uint16_t str_len_ = 0;
    uint32_t unicode_ = 0;
    uint8_t unicode_digits_ = 0;
    uint32_t high_surrogate_ = 0;
    char num_[kMaxNumber + 1] = {};
    uint8_t num_len_ = 0;
    const char* literal_ = "";
    uint8_t literal_pos_ = 0;
    JsonType literal_type_ = JsonType::Null;
};

}  // namespace nvp::providers
