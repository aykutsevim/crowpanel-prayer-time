// Prints the prayer-time engine's output for the shared fixture as a Markdown table
// (spec 002 FR-022; expected output: specs/002-prayer-engine/contracts/engine-table.md).
// Build: pio run -e engine_table. Run and check: python tools/check_engine_table.py
#include <cstdio>

#include "nvp/engine/engine.h"
#include "nvp/engine/fixture.h"
#include "nvp/engine/table.h"

int main() {
    using namespace nvp::engine;
    // The moments listed in docs/development/prompts/slice-02-prayer-engine.md, step 7.
    const int moments[][3] = {{3, 30, 0},  {6, 15, 59}, {6, 16, 0},  {6, 50, 0},  {10, 0, 0},
                              {12, 54, 0}, {12, 55, 0}, {14, 32, 7}, {16, 20, 0}, {17, 42, 0},
                              {17, 51, 0}, {18, 20, 0}, {19, 14, 0}, {21, 30, 0}, {23, 59, 59}};
    static char line[2048];
    table_header(line, sizeof line);
    std::fputs(line, stdout);
    for (const auto& m : moments) {
        Input in;
        in.now = Moment{fixture::kDate, m[0] * 3600 + m[1] * 60 + m[2]};
        in.yesterday = &fixture::kYesterday;
        in.today = &fixture::kToday;
        in.tomorrow = &fixture::kTomorrow;
        MainView v;
        evaluate(in, v);
        if (table_row(v, line, sizeof line) == 0) return 1;
        std::fputs(line, stdout);
    }
    return 0;
}
