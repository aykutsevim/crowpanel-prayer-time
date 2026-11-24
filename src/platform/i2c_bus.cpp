#include "i2c_bus.h"

#include <driver/i2c_master.h>

#include "board.h"

namespace platform::i2c {
namespace {

constexpr int kTimeoutMs = 50;
constexpr int kMaxDevices = 4;

i2c_master_bus_handle_t g_bus = nullptr;

struct Slot {
    uint8_t address = 0;
    i2c_master_dev_handle_t handle = nullptr;
};
Slot g_devices[kMaxDevices];

i2c_master_dev_handle_t device(uint8_t address) {
    if (g_bus == nullptr) return nullptr;
    for (Slot& s : g_devices) {
        if (s.handle != nullptr && s.address == address) return s.handle;
    }
    for (Slot& s : g_devices) {
        if (s.handle == nullptr) {
            i2c_device_config_t cfg = {};
            cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
            cfg.device_address = address;
            cfg.scl_speed_hz = board::kI2cHz;
            if (i2c_master_bus_add_device(g_bus, &cfg, &s.handle) != ESP_OK) return nullptr;
            s.address = address;
            return s.handle;
        }
    }
    return nullptr;
}

}  // namespace

bool begin() {
    if (g_bus != nullptr) return true;
    i2c_master_bus_config_t cfg = {};
    cfg.i2c_port = I2C_NUM_0;
    cfg.sda_io_num = static_cast<gpio_num_t>(board::kI2cSda);
    cfg.scl_io_num = static_cast<gpio_num_t>(board::kI2cScl);
    cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    cfg.glitch_ignore_cnt = 7;
    cfg.flags.enable_internal_pullup = true;
    return i2c_new_master_bus(&cfg, &g_bus) == ESP_OK;
}

bool probe(uint8_t address) { return g_bus != nullptr && i2c_master_probe(g_bus, address, kTimeoutMs) == ESP_OK; }

bool write(uint8_t address, const uint8_t* data, size_t n) {
    i2c_master_dev_handle_t d = device(address);
    return d != nullptr && i2c_master_transmit(d, data, n, kTimeoutMs) == ESP_OK;
}

bool write_read(uint8_t address, const uint8_t* wdata, size_t wn, uint8_t* rdata, size_t rn) {
    i2c_master_dev_handle_t d = device(address);
    return d != nullptr && i2c_master_transmit_receive(d, wdata, wn, rdata, rn, kTimeoutMs) == ESP_OK;
}

}  // namespace platform::i2c
