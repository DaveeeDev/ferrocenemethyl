#pragma once
#include "driver/gpio.h"
#include "driver/twai.h"
#include "SerialLogger.hpp"

namespace BoardConfig {
    // ESP32-C3 safe pins (avoids GPIO18/19 USB-JTAG and GPIO4-7 JTAG)
    constexpr gpio_num_t kCanTxPin = GPIO_NUM_0;
    constexpr gpio_num_t kCanRxPin = GPIO_NUM_1;

    // Listen-Only prevents transmitting ACKs or error frames while sniffing
    constexpr twai_mode_t kCanMode = TWAI_MODE_LISTEN_ONLY;

    // FreeRTOS RX task configuration
    constexpr uint32_t    kRxTaskStackSize = 4096;
    constexpr UBaseType_t kRxTaskPriority  = 6;

    // Active consumer swap point: replace SerialLogger with your future RuleEvaluator here
    using FrameConsumer = SerialLogger;
}