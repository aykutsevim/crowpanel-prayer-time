#include "tasks.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

namespace platform::tasks {

bool start_worker(const char* name, int core, uint32_t stack_bytes, WorkerFn fn, void* arg) {
    TaskHandle_t handle = nullptr;
    const BaseType_t ok = xTaskCreatePinnedToCore(fn, name, stack_bytes, arg, 1, &handle, core);
    return ok == pdPASS;
}

void sleep_ms(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

Lock::Lock() : handle_(xSemaphoreCreateMutex()) {}

void Lock::lock() { xSemaphoreTake(static_cast<SemaphoreHandle_t>(handle_), portMAX_DELAY); }

void Lock::unlock() { xSemaphoreGive(static_cast<SemaphoreHandle_t>(handle_)); }

}  // namespace platform::tasks
