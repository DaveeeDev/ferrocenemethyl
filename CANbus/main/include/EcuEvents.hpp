#pragma once
#include "CanTypes.hpp"
#include <cstddef>
#include <cstdint>

class RelayDriver;


// ECU OPERATING STATES
// Add new states here (e.g., COURTESY_TIMER, CONFIG_MODE, DEEP_SLEEP).
// Keep EcuState::ANY last (0xFF) for wildcard transition matching.
enum class EcuState : uint8_t {
    INIT = 0,
    STANDBY,
    ACTIVE,
    ERROR,
    ANY = 0xFF
};

inline const char* stateToString(EcuState state) {
    switch (state) {
        case EcuState::INIT:    return "INIT";
        case EcuState::STANDBY: return "STANDBY";
        case EcuState::ACTIVE:  return "ACTIVE";
        case EcuState::ERROR:   return "ERROR";
        case EcuState::ANY:     return "ANY";
        default:                return "UNKNOWN";
    }
}

// ECU EVENTS (Posted into the State Machine Queue)
enum class EventId : uint8_t {
    EVT_BOOT_DONE = 0,
    EVT_IGNITION_ON,
    EVT_IGNITION_OFF,
    EVT_SIGNAL_UPDATED, // Any mapped CAN signal changed state (e.g., HIGH_BEAM)
    EVT_CAN_BUS_OFF,
    EVT_CAN_HEALTHY,
    EVT_CAN_TIMEOUT     // Heartbeat watchdog expired (no valid CAN frames received)
};

inline const char* eventToString(EventId id) {
    switch (id) {
        case EventId::EVT_BOOT_DONE:      return "EVT_BOOT_DONE";
        case EventId::EVT_IGNITION_ON:    return "EVT_IGNITION_ON";
        case EventId::EVT_IGNITION_OFF:   return "EVT_IGNITION_OFF";
        case EventId::EVT_SIGNAL_UPDATED: return "EVT_SIGNAL_UPDATED";
        case EventId::EVT_CAN_BUS_OFF:    return "EVT_CAN_BUS_OFF";
        case EventId::EVT_CAN_HEALTHY:    return "EVT_CAN_HEALTHY";
        case EventId::EVT_CAN_TIMEOUT:    return "EVT_CAN_TIMEOUT";
        default:                          return "EVT_UNKNOWN";
    }
}

struct EcuEvent {
    EventId       id;
    VehicleSignal signal;       // Relevant when id == EVT_SIGNAL_UPDATED (or ignition)
    bool          signalActive;
    uint32_t      timestampMs;
};

// STATE MACHINE CONTEXT & TABLE SCHEMAS
// Owned exclusively by EcuStateMachineTask
struct EcuContext {
    static constexpr size_t kMaxSignals = 16;

    RelayDriver& relays;
    EcuState     currentState = EcuState::INIT;
    bool         signals[kMaxSignals] = {};

    bool getSignal(VehicleSignal sig) const {
        const size_t idx = static_cast<size_t>(sig);
        return (idx < kMaxSignals) ? signals[idx] : false;
    }

    void setSignal(VehicleSignal sig, bool active) {
        const size_t idx = static_cast<size_t>(sig);
        if (idx < kMaxSignals) {
            signals[idx] = active;
        }
    }

    void clearSignals() {
        for (size_t i = 0; i < kMaxSignals; ++i) {
            signals[i] = false;
        }
    }
};

using StateLifecycleFn  = void (*)(EcuContext& ctx);
using TransitionActionFn = void (*)(EcuContext& ctx, const EcuEvent& evt);

struct StateHandler {
    EcuState         state;
    StateLifecycleFn onEnter; // Executed once whenever entering this state
    StateLifecycleFn onExit;  // Executed once whenever leaving this state
};

struct Transition {
    EcuState           currentState; // Specific state or EcuState::ANY
    EventId            event;
    EcuState           nextState;
    TransitionActionFn action;       // Optional action executed during transition
};