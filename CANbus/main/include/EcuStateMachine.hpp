#pragma once
#include "EcuEvents.hpp"
#include "RelayDriver.hpp"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"

class EcuStateMachine {
public:
    explicit EcuStateMachine(RelayDriver& relays);

    esp_err_t start();
    bool      postEvent(const EcuEvent& evt, TickType_t ticksToWait = 0);
    EcuState  getState() const { return ctx_.currentState; }

private:
    static constexpr uint32_t    kTaskStackSize      = 4096;
    static constexpr UBaseType_t kTaskPriority       = 5;
    static constexpr size_t      kQueueLength        = 32;
    static constexpr uint32_t    kCanTimeoutMs       = 500; // Failsafe if bus goes silent

    static void taskEntry(void* arg);
    static void heartbeatTimerCb(TimerHandle_t timer);

    void runLoop();
    void dispatch(const EcuEvent& evt);
    const Transition* findTransition(EcuState state, EventId event) const;
    const StateHandler* findStateHandler(EcuState state) const;

    // State lifecycle hooks (onEnter / onExit)
    static void onEnterStandby(EcuContext& ctx);
    static void onEnterActive(EcuContext& ctx);
    static void onExitActive(EcuContext& ctx);
    static void onEnterError(EcuContext& ctx);

    // Transition actions
    static void actionEvaluateOutputs(EcuContext& ctx, const EcuEvent& evt);

    EcuContext    ctx_;
    QueueHandle_t eventQueue_     = nullptr;
    TaskHandle_t  taskHandle_     = nullptr;
    TimerHandle_t heartbeatTimer_ = nullptr;

    static const StateHandler kStateHandlers[];
    static const Transition   kTransitionTable[];
};