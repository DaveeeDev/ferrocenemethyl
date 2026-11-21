#pragma once
#include "driver/gpio.h"
#include "driver/twai.h"
#include "SerialLogger.hpp"
// #include "SignalTriggerConsumer.hpp" // Uncomment when switching to ECU mode

namespace BoardConfig {
    // ESP32-C3 safe pins
    constexpr gpio_num_t kCanTxPin        = GPIO_NUM_0;
    constexpr gpio_num_t kCanRxPin        = GPIO_NUM_1;
    constexpr gpio_num_t kRelay1Pin       = GPIO_NUM_3;
    constexpr gpio_num_t kRelay2Pin       = GPIO_NUM_10;
    constexpr bool       kRelayActiveHigh = true;        // true = MOSFET driver, false = opto-relay board

    // Listen-Only prevents transmitting ACKs or error frames while sniffing
    constexpr twai_mode_t kCanMode = TWAI_MODE_LISTEN_ONLY;

    // FreeRTOS RX task configuration
    constexpr uint32_t    kRxTaskStackSize = 4096;
    constexpr UBaseType_t kRxTaskPriority  = 6;

    // Active consumer swap point, replace SerialLogger with SignalTriggerConsumer for final ECU
    using FrameConsumer = SerialLogger;
}