#pragma once
#include "CanTypes.hpp"

class SerialLogger {
public:
    void printHeader(const char* platformName, bool is125Kbps) const;
    void onFrameReceived(const CanFrame& frame);
    void onBusStateChanged(bool healthy);
};