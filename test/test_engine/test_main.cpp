// Test runner for the prayer-time engine (lib/core/src/nvp/engine, spec 002).
// PlatformIO builds every file of a test folder into one program, so each file exposes a
// run_*() function and this file owns main(), setUp() and tearDown().
//
// It also replaces the global allocation operators with counting versions, so the sweep test can
// prove that evaluate() never allocates (FR-023, SC-007, research R7).
#include <unity.h>

#include <cstddef>
#include <cstdlib>
#include <new>

std::size_t g_allocations = 0;

void* operator new(std::size_t n) {
    ++g_allocations;
    if (void* p = std::malloc(n == 0 ? 1 : n)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) { return operator new(n); }
void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
    ++g_allocations;
    return std::malloc(n == 0 ? 1 : n);
}
void* operator new[](std::size_t n, const std::nothrow_t& t) noexcept { return operator new(n, t); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

void run_calendar_tests();
void run_text_tests();
void run_record_tests();
void run_period_tests();
void run_column_tests();
void run_day_tests();
void run_view_text_tests();
void run_kerahat_tests();
void run_sweep_tests();
void run_table_tests();

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    run_calendar_tests();
    run_text_tests();
    run_record_tests();
    run_period_tests();
    run_column_tests();
    run_day_tests();
    run_view_text_tests();
    run_kerahat_tests();
    run_sweep_tests();
    run_table_tests();
    return UNITY_END();
}
