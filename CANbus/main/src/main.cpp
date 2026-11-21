#include "BoardConfig.hpp"
#include "CanEngine.hpp"
#include "VolvoP3.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdio>

extern "C" void app_main(void) {
    // 5s countdown to connect serial monitor before traffic starts
    for (int i = 5; i > 0; --i) {
        std::printf("Starting CAN sniffer in %d...\n", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    std::printf("Started CAN sniffer!\n");

    static BoardConfig::FrameConsumer consumer;
    static CanEngine canEngine(consumer);

    consumer.printHeader(PlatformConfig::kName, PlatformConfig::k125Kbps);

    if (canEngine.start() != ESP_OK) {
        std::printf("FATAL: Failed to start CanEngine. Check transceiver wiring.\n");
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}