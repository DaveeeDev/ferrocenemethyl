#include "EcuStateMachine.hpp"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include <cstdio>

static const char* TAG = "EcuFSM";

struct OutputRule {
    RelayChannel  channel;
    VehicleSignal triggerSignal;
};

static constexpr OutputRule kOutputRules[] = {
    { RelayChannel::RELAY_1, VehicleSignal::HIGH_BEAM    },
    { RelayChannel::RELAY_2, VehicleSignal::REVERSE_GEAR },
};

// STATE LIFECYCLE HANDLERS (onEnter / onExit)
// Add new state entry/exit behaviors here when adding new states
const StateHandler EcuStateMachine::kStateHandlers[] = {
    // State              onEnter                          onExit
    { EcuState::INIT,     nullptr,                         nullptr                       },
    { EcuState::STANDBY,  &EcuStateMachine::onEnterStandby,nullptr                       },
    { EcuState::ACTIVE,   &EcuStateMachine::onEnterActive, &EcuStateMachine::onExitActive },
    { EcuState::ERROR,    &EcuStateMachine::onEnterError,  nullptr                       },
};

// STATE TRANSITION TABLE
// Specific state rows are checked first; EcuState::ANY acts as a wildcard.
const Transition EcuStateMachine::kTransitionTable[] = {
    // Current State       Incoming Event               Next State          Action
    { EcuState::INIT,      EventId::EVT_BOOT_DONE,      EcuState::STANDBY,  nullptr                                  },

    { EcuState::STANDBY,   EventId::EVT_IGNITION_ON,    EcuState::ACTIVE,   nullptr                                  },

    { EcuState::ACTIVE,    EventId::EVT_SIGNAL_UPDATED, EcuState::ACTIVE,   &EcuStateMachine::actionEvaluateOutputs  },
    { EcuState::ACTIVE,    EventId::EVT_IGNITION_OFF,   EcuState::STANDBY,  nullptr                                  },

    // Wildcard Fault Protection: applies automatically from ANY state
    { EcuState::ANY,       EventId::EVT_CAN_BUS_OFF,    EcuState::ERROR,    nullptr                                  },
    { EcuState::ANY,       EventId::EVT_CAN_TIMEOUT,    EcuState::ERROR,    nullptr                                  },

    // Recovery from ERROR returns safely to STANDBY until fresh IGNITION_ON arrives
    { EcuState::ERROR,     EventId::EVT_CAN_HEALTHY,    EcuState::STANDBY,  nullptr                                  },
};

EcuStateMachine::EcuStateMachine(RelayDriver& relays)
    : ctx_{ relays, EcuState::INIT, {} } {}

esp_err_t EcuStateMachine::start() {
    ctx_.relays.init(); // Enforce OFF before starting RTOS objects

    eventQueue_ = xQueueCreate(kQueueLength, sizeof(EcuEvent));
    if (!eventQueue_) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return ESP_ERR_NO_MEM;
    }

    heartbeatTimer_ = xTimerCreate(
        "can_hb_tmr",
        pdMS_TO_TICKS(kCanTimeoutMs),
        pdFALSE, // One shot timer; reset whenever valid CAN traffic arrives
        this,
        heartbeatTimerCb);

    if (!heartbeatTimer_) {
        ESP_LOGE(TAG, "Failed to create heartbeat timer");
        return ESP_ERR_NO_MEM;
    }

    BaseType_t res = xTaskCreate(
        taskEntry, "ecu_fsm_task", kTaskStackSize, this, kTaskPriority, &taskHandle_);

    if (res != pdPASS) {
        ESP_LOGE(TAG, "Failed to create FSM task");
        return ESP_FAIL;
    }

    // Kick off initial transition: INIT -> STANDBY
    postEvent({ EventId::EVT_BOOT_DONE, VehicleSignal::IGNITION_ON, false, 0 });
    return ESP_OK;
}

bool EcuStateMachine::postEvent(const EcuEvent& evt, TickType_t ticksToWait) {
    if (!eventQueue_) {
        return false;
    }
    return xQueueSend(eventQueue_, &evt, ticksToWait) == pdTRUE;
}

void EcuStateMachine::taskEntry(void* arg) {
    static_cast<EcuStateMachine*>(arg)->runLoop();
}

void EcuStateMachine::heartbeatTimerCb(TimerHandle_t timer) {
    auto* self = static_cast<EcuStateMachine*>(pvTimerGetTimerID(timer));
    if (self) {
        const uint32_t nowMs = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
        self->postEvent({ EventId::EVT_CAN_TIMEOUT, VehicleSignal::IGNITION_ON, false, nowMs });
    }
}

void EcuStateMachine::runLoop() {
    esp_task_wdt_add(nullptr);

    EcuEvent evt{};
    while (true) {
        if (xQueueReceive(eventQueue_, &evt, pdMS_TO_TICKS(200)) == pdTRUE) {
            dispatch(evt);
        }
        esp_task_wdt_reset();
    }
}

void EcuStateMachine::dispatch(const EcuEvent& evt) {
    if (evt.id == EventId::EVT_IGNITION_ON ||
        evt.id == EventId::EVT_IGNITION_OFF ||
        evt.id == EventId::EVT_SIGNAL_UPDATED) {
        ctx_.setSignal(evt.signal, evt.signalActive);
        xTimerReset(heartbeatTimer_, 0);
    } else if (evt.id == EventId::EVT_CAN_HEALTHY) {
        xTimerReset(heartbeatTimer_, 0);
    }

    // Look up exact (currentState, event) match first, then fallback to ANY
    const Transition* transition = findTransition(ctx_.currentState, evt.id);
    if (!transition) {
        transition = findTransition(EcuState::ANY, evt.id);
    }

    if (!transition) {
        return; // Event ignored in current state
    }

    const EcuState prevState = ctx_.currentState;
    const EcuState nextState = transition->nextState;
    const bool     changing  = (prevState != nextState);

    if (changing) {
        std::printf("[%8lu ms] [FSM] %s --(%s)--> %s\n",
                    static_cast<unsigned long>(evt.timestampMs),
                    stateToString(prevState),
                    eventToString(evt.id),
                    stateToString(nextState));

        const StateHandler* prevHandler = findStateHandler(prevState);
        if (prevHandler && prevHandler->onExit) {
            prevHandler->onExit(ctx_);
        }
    }

    if (transition->action) {
        transition->action(ctx_, evt);
    }

    if (changing) {
        ctx_.currentState = nextState;
        const StateHandler* nextHandler = findStateHandler(nextState);
        if (nextHandler && nextHandler->onEnter) {
            nextHandler->onEnter(ctx_);
        }
    }
}

const Transition* EcuStateMachine::findTransition(EcuState state, EventId event) const {
    for (const auto& row : kTransitionTable) {
        if (row.currentState == state && row.event == event) {
            return &row;
        }
    }
    return nullptr;
}

const StateHandler* EcuStateMachine::findStateHandler(EcuState state) const {
    for (const auto& handler : kStateHandlers) {
        if (handler.state == state) {
            return &handler;
        }
    }
    return nullptr;
}

// ACTION IMPLEMENTATIONS

void EcuStateMachine::onEnterStandby(EcuContext& ctx) {
    ctx.relays.setAllOff();
}

void EcuStateMachine::onEnterActive(EcuContext& ctx) {
    // Evaluate outputs immediately upon entering ACTIVE (e.g., if high beam was already ON)
    EcuEvent dummy{};
    actionEvaluateOutputs(ctx, dummy);
}

void EcuStateMachine::onExitActive(EcuContext& ctx) {
    // Guarantee outputs turn off whenever leaving ACTIVE (to STANDBY, ERROR, or future states)
    ctx.relays.setAllOff();
}

void EcuStateMachine::onEnterError(EcuContext& ctx) {
    xTimerStop(nullptr, 0);
    ctx.clearSignals();
    ctx.relays.setAllOff();
}

void EcuStateMachine::actionEvaluateOutputs(EcuContext& ctx, const EcuEvent&) {
    if (ctx.currentState != EcuState::ACTIVE) {
        return;
    }

    for (const auto& rule : kOutputRules) {
        const bool shouldEnergize = ctx.getSignal(rule.triggerSignal);
        ctx.relays.setChannel(rule.channel, shouldEnergize);
    }
}