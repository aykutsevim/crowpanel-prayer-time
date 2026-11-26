// JsonStream (spec 006 T003, research R2, FR-003): key paths, chunking, escapes, limits, errors.
#include <unity.h>

#include <cstring>
#include <string>
#include <vector>

#include "nvp/providers/json_stream.h"

using nvp::providers::JsonError;
using nvp::providers::JsonEvent;
using nvp::providers::JsonSink;
using nvp::providers::JsonStream;
using nvp::providers::JsonType;

namespace {

const char* type_name(JsonType t) {
    switch (t) {
        case JsonType::BeginObject: return "{";
        case JsonType::EndObject: return "}";
        case JsonType::BeginArray: return "[";
        case JsonType::EndArray: return "]";
        case JsonType::String: return "s";
        case JsonType::Number: return "n";
        case JsonType::True: return "t";
        case JsonType::False: return "f";
        case JsonType::Null: return "0";
    }
    return "?";
}

// Records every event as "<type> <path>=<text>[!]" (! = overflow).
struct Recorder : JsonSink {
    std::vector<std::string> events;
    void on_json(const JsonEvent& e) override {
        std::string s = type_name(e.type);
        s += " ";
        s += e.path;
        if (e.type == JsonType::String || e.type == JsonType::Number) {
            s += "=";
            s += std::string(e.text, e.length);
            TEST_ASSERT_EQUAL_size_t(std::strlen(e.text), e.length);
        }
        if (e.overflow) s += "!";
        events.push_back(s);
    }
};

struct Run {
    std::vector<std::string> events;
    bool fed_ok = true;
    bool finished = false;
    JsonError error = JsonError::None;
};

Run run_chunked(const std::string& doc, size_t chunk) {
    Recorder rec;
    JsonStream js(rec);
    Run r;
    for (size_t at = 0; at < doc.size(); at += chunk) {
        size_t n = doc.size() - at < chunk ? doc.size() - at : chunk;
        if (!js.feed(doc.data() + at, n)) {
            r.fed_ok = false;
            break;
        }
    }
    r.finished = js.finish();
    r.error = js.error();
    r.events = rec.events;
    return r;
}

const char* kSmall =
    "{\"success\":true,\"data\":{\"location\":{\"district_id\":\"9206\"},"
    "\"times\":[{\"date\":\"2026-11-24\",\"hijri\":{\"day\":15,\"year\":1448},\"imsak\":\"06:08\",\"flags\":[]},"
    "{\"date\":\"2026-11-25\",\"n\":-1.5e3,\"z\":null,\"f\":false}]},\"meta\":{\"count\":2}}";

const std::vector<std::string> kSmallEvents = {
    "{ ",
    "t success",
    "{ data",
    "{ data.location",
    "s data.location.district_id=9206",
    "} data.location",
    "[ data.times",
    "{ data.times[]",
    "s data.times[].date=2026-11-24",
    "{ data.times[].hijri",
    "n data.times[].hijri.day=15",
    "n data.times[].hijri.year=1448",
    "} data.times[].hijri",
    "s data.times[].imsak=06:08",
    "[ data.times[].flags",
    "] data.times[].flags",
    "} data.times[]",
    "{ data.times[]",
    "s data.times[].date=2026-11-25",
    "n data.times[].n=-1.5e3",
    "0 data.times[].z",
    "f data.times[].f",
    "} data.times[]",
    "] data.times",
    "} data",
    "{ meta",
    "n meta.count=2",
    "} meta",
    "} ",
};

void expect_events(const std::vector<std::string>& want, const std::vector<std::string>& got) {
    TEST_ASSERT_EQUAL_size_t(want.size(), got.size());
    for (size_t i = 0; i < want.size(); ++i) TEST_ASSERT_EQUAL_STRING(want[i].c_str(), got[i].c_str());
}

void test_paths_and_values() {  // R2
    Run r = run_chunked(kSmall, 100000);
    TEST_ASSERT_TRUE(r.fed_ok);
    TEST_ASSERT_TRUE(r.finished);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(JsonError::None), static_cast<int>(r.error));
    expect_events(kSmallEvents, r.events);
}

void test_chunk_sizes_give_same_events() {  // R2: a token may split between chunks
    const size_t sizes[] = {1, 7, 64, 1024};
    for (size_t s : sizes) {
        Run r = run_chunked(kSmall, s);
        TEST_ASSERT_TRUE(r.finished);
        expect_events(kSmallEvents, r.events);
    }
}

void test_escapes() {
    // \" \\ \/ \n, \u00e7 (ç), a raw UTF-8 ç, and a surrogate pair (U+1F600).
    std::string doc = "{\"a\":\"x\\\"y\\\\z\\/\\n\",\"b\":\"\\u00e7\",\"c\":\"\xc3\xa7\",\"d\":\"\\ud83d\\ude00\"}";
    for (size_t chunk : {size_t{1}, size_t{3}, size_t{1000}}) {
        Run r = run_chunked(doc, chunk);
        TEST_ASSERT_TRUE(r.finished);
        TEST_ASSERT_EQUAL_size_t(6, r.events.size());
        TEST_ASSERT_EQUAL_STRING("s a=x\"y\\z/\n", r.events[1].c_str());
        TEST_ASSERT_EQUAL_STRING("s b=\xc3\xa7", r.events[2].c_str());
        TEST_ASSERT_EQUAL_STRING("s c=\xc3\xa7", r.events[3].c_str());
        TEST_ASSERT_EQUAL_STRING("s d=\xf0\x9f\x98\x80", r.events[4].c_str());
    }
}

void test_long_value_is_flagged_not_kept() {  // R2: no more than 64 bytes are held
    std::string v(65, 'x');
    Run r = run_chunked("{\"k\":\"" + v + "\"}", 5);
    TEST_ASSERT_TRUE(r.finished);  // a parser decides whether that field matters
    TEST_ASSERT_EQUAL_STRING(("s k=" + std::string(64, 'x') + "!").c_str(), r.events[1].c_str());
    Run ok = run_chunked("{\"k\":\"" + std::string(64, 'x') + "\"}", 5);
    TEST_ASSERT_EQUAL_STRING(("s k=" + std::string(64, 'x')).c_str(), ok.events[1].c_str());
}

void expect_error(const std::string& doc, JsonError want) {
    for (size_t chunk : {size_t{1}, size_t{1000}}) {
        Run r = run_chunked(doc, chunk);
        TEST_ASSERT_FALSE_MESSAGE(r.finished, doc.c_str());
        TEST_ASSERT_EQUAL_INT_MESSAGE(static_cast<int>(want), static_cast<int>(r.error), doc.c_str());
    }
}

void test_errors() {  // R2: malformed or truncated input ends the parse with an error
    expect_error("{\"" + std::string(65, 'k') + "\":1}", JsonError::KeyTooLong);
    expect_error("[[[[[[[[[1]]]]]]]]]", JsonError::TooDeep);  // depth 9
    Run eight = run_chunked("[[[[[[[[1]]]]]]]]", 1);           // depth 8 is fine
    TEST_ASSERT_TRUE(eight.finished);
    expect_error("{\"a\":1", JsonError::Truncated);  // missing }
    expect_error("{\"a\":\"abc", JsonError::Truncated);
    expect_error("", JsonError::Truncated);
    expect_error("{\"a\":1} x", JsonError::TrailingData);
    expect_error("{\"a\":1}{}", JsonError::TrailingData);
    expect_error("{\"a\" 1}", JsonError::Syntax);
    expect_error("{\"a\":1,}", JsonError::Syntax);
    expect_error("[1,]", JsonError::Syntax);
    expect_error("{\"a\":tru}", JsonError::Syntax);
    expect_error("{\"a\":\"\\q\"}", JsonError::Syntax);
    expect_error("{1:2}", JsonError::Syntax);
    expect_error("]", JsonError::Syntax);
}

void test_top_level_scalars() {
    Run r = run_chunked(" 42 ", 1);
    TEST_ASSERT_TRUE(r.finished);
    TEST_ASSERT_EQUAL_STRING("n =42", r.events[0].c_str());
    Run e = run_chunked("[]", 1);
    TEST_ASSERT_TRUE(e.finished);
    TEST_ASSERT_EQUAL_size_t(2, e.events.size());
}

}  // namespace

void run_json_stream_tests() {
    RUN_TEST(test_paths_and_values);
    RUN_TEST(test_chunk_sizes_give_same_events);
    RUN_TEST(test_escapes);
    RUN_TEST(test_long_value_is_flagged_not_kept);
    RUN_TEST(test_errors);
    RUN_TEST(test_top_level_scalars);
}
