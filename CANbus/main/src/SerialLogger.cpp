#include "SerialLogger.hpp"
#include <cstdio>

void SerialLogger::printHeader(const char* platformName, bool is125Kbps) const {
    std::printf("\n=== CAN Sniffer Active | %s (%s kbps, Listen-Only) ===\n",
                platformName, is125Kbps ? "125" : "500");
    std::printf("Timestamp (ms) | ID       | DLC | Data Hex\n");
    std::printf("---------------+----------+-----+------------------------\n");
}

void SerialLogger::onFrameReceived(const CanFrame& frame) {
    if (frame.isExtended) {
        std::printf("%14lu | %08lX |  %u  | ",
                    static_cast<unsigned long>(frame.timestampMs),
                    static_cast<unsigned long>(frame.id),
                    frame.dlc);
    } else {
        std::printf("%14lu |      %03lX |  %u  | ",
                    static_cast<unsigned long>(frame.timestampMs),
                    static_cast<unsigned long>(frame.id),
                    frame.dlc);
    }

    if (frame.isRtr) {
        std::printf("RTR\n");
        return;
    }

    for (uint8_t i = 0; i < frame.dlc && i < 8; ++i) {
        std::printf("%02X ", frame.data[i]);
    }
    std::printf("\n");
}

void SerialLogger::onBusStateChanged(bool healthy) {
    std::printf("[BUS ALERT] CAN bus is now %s\n", healthy ? "HEALTHY" : "BUS_OFF");
}