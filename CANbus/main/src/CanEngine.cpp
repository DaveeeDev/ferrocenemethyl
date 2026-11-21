#include "CanEngine.hpp"
#include "VolvoP3.hpp"
#include "driver/twai.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char* TAG = "CanEngine";

CanEngine::CanEngine(BoardConfig::FrameConsumer& consumer) : consumer_(consumer) {}

esp_err_t CanEngine::start() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        BoardConfig::kCanTxPin, BoardConfig::kCanRxPin, BoardConfig::kCanMode);
    g_config.rx_queue_len = 64;

    twai_timing_config_t t_config = PlatformConfig::k125Kbps
        ? (twai_timing_config_t)TWAI_TIMING_CONFIG_125KBITS()
        : (twai_timing_config_t)TWAI_TIMING_CONFIG_500KBITS();

    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "TWAI driver install failed: %s", esp_err_to_name(err));
        return err;
    }

    uint32_t alerts = TWAI_ALERT_BUS_OFF | TWAI_ALERT_BUS_RECOVERED |
                      TWAI_ALERT_ERR_PASS | TWAI_ALERT_RX_QUEUE_FULL;
    twai_reconfigure_alerts(alerts, nullptr);

    err = twai_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "TWAI start failed: %s", esp_err_to_name(err));
        return err;
    }

    BaseType_t res = xTaskCreate(
        rxTaskEntry, "can_rx_task",
        BoardConfig::kRxTaskStackSize, this,
        BoardConfig::kRxTaskPriority, &rxTaskHandle_);

    return (res == pdPASS) ? ESP_OK : ESP_FAIL;
}

void CanEngine::rxTaskEntry(void* arg) {
    static_cast<CanEngine*>(arg)->rxLoop();
}

void CanEngine::rxLoop() {
    twai_message_t rawMsg;

    while (true) {
        if (twai_receive(&rawMsg, pdMS_TO_TICKS(50)) == ESP_OK) {
            CanFrame frame{};
            frame.timestampMs = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
            frame.id          = rawMsg.identifier;
            frame.dlc         = rawMsg.data_length_code;
            frame.isExtended  = rawMsg.extd;
            frame.isRtr       = rawMsg.rtr;

            for (uint8_t i = 0; i < frame.dlc && i < 8; ++i) {
                frame.data[i] = rawMsg.data[i];
            }

            consumer_.onFrameReceived(frame);
        }

        checkAlerts();
    }
}

void CanEngine::checkAlerts() {
    uint32_t triggered = 0;
    if (twai_read_alerts(&triggered, 0) != ESP_OK || triggered == 0) {
        return;
    }

    if (triggered & TWAI_ALERT_BUS_OFF) {
        if (busHealthy_) {
            busHealthy_ = false;
            consumer_.onBusStateChanged(false);
        }
        twai_initiate_recovery();
    }

    if (triggered & TWAI_ALERT_BUS_RECOVERED) {
        twai_start();
        busHealthy_ = true;
        consumer_.onBusStateChanged(true);
    }
}