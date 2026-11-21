#pragma once
#include "BoardConfig.hpp"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class CanEngine {
public:
    explicit CanEngine(BoardConfig::FrameConsumer& consumer);
    esp_err_t start();

private:
    static void rxTaskEntry(void* arg);
    void rxLoop();
    void checkAlerts();

    BoardConfig::FrameConsumer& consumer_;
    TaskHandle_t                rxTaskHandle_ = nullptr;
    bool                        busHealthy_   = true;
};