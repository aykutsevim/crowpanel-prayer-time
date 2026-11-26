// Tasks and locking for src/app without FreeRTOS headers there (constitution VI, plan.md).
// Network, HTTPS and flash work run on core 0; LVGL and the display on core 1 (research R8).
#pragma once

#include <cstddef>
#include <cstdint>

namespace platform::tasks {

using WorkerFn = void (*)(void* arg);

// Starts a task that runs `fn(arg)` forever on `core`. Returns false if it could not start.
bool start_worker(const char* name, int core, uint32_t stack_bytes, WorkerFn fn, void* arg);

void sleep_ms(uint32_t ms);

// Zeroed memory in PSRAM for large, long-lived buffers (spec 006: the day store). nullptr on failure.
void* alloc_psram(size_t bytes);

// A mutex for state shared between the two cores.
class Lock {
public:
    Lock();
    void lock();
    void unlock();

private:
    void* handle_;
};

class Guard {
public:
    explicit Guard(Lock& l) : l_(l) { l_.lock(); }
    ~Guard() { l_.unlock(); }
    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;

private:
    Lock& l_;
};

}  // namespace platform::tasks
